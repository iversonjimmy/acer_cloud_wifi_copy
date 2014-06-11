//
//  Copyright 2011-2013 Acer Cloud Technology.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF Acer Cloud Technology.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF Acer Cloud Technology.
//

#include <vpl_time.h>
#include <vpl_net.h>
#include <vplu.h>
#include <vpl_conv.h>
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

#include "pxd_client.h"
#include "pxd_log.h"
#include "pxd_mtwist.h"
#include "pxd_util.h"
#include "pxd_event.h"
#include "pxd_packet.h"

#undef  true
#undef  false
#undef  null

#define  true    1
#define  false   0
#define  null    0

#define pxd_abs(x)         ((x) >= 0 ? (x) : -(x))
#define pxd_min(a, b)      ((a) < (b) ? (a) : (b))
#define pxd_max(a, b)      ((a) > (b) ? (a) : (b))
#define array_size(x)  (sizeof(x) / sizeof((x)[0]))

/*
 *  Define the AnsCommon packet types for which this
 *  module has code.
 */
#define Send_subscriptions      33
#define Request_sleep_setup     54
#define Send_sleep_setup        53
#define Send_ping               35
#define Send_timed_ping         12
#define Send_login_blob         26
#define Send_challenge          27
#define Send_device_shutdown    67
#define Set_device_params       37
#define Request_wakeup          34
#define Query_device_list       39
#define Reject_credentials      57
#define Set_login_version       62
#define Set_param_version       63
#define Send_state_list          1
#define Send_response            9
#define Send_unicast             6
#define Send_multicast           7
#define Send_device_update      43
#define Send_state_compat       42
#define Set_device_compat       36

#define pxd_config_params       11

#define DEVICE_ONLINE    1
#define DEVICE_SLEEPING  2
#define DEVICE_OFFLINE   3
#define QUERY_FAILED     4

#define header_size       30
#define tag_offset         6
#define sequence_offset   22
#define signature_size    CSL_SHA1_DIGESTSIZE
#define state_size         9

#define blob_version       3
#define blob_fields        5
#define blob_count        44

#define demon_version      3
#define demon_count       28
#define demon_fields       5

#define max_device_state  30  /* from AnsCommon.java */

/*
 *  Define the block size and the control structure for encryption.  We
 *  use AES.
 */
#define aes_block_size 16

struct pxd_crypto_s {
    u8               sha1_key[20];
    u8               aes_key [aes_block_size];
    mtwist_t         mt;
};

/*
 *  Define some macros to unpack values from a packet.  Integer
 *  values are sent in network byte order.
 */
#define unpack_u8(x)                                \
        uint8_t x;                                  \
                                                    \
        do {                                        \
            fail |= end - base <= 0;                \
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

#define unpack_u8_into(dest)                        \
        do {                                        \
            unpack_u8(temp);                        \
            (dest) = temp;                          \
        } while (0)

#define unpack_u16_into(dest)                       \
        do {                                        \
            unpack_u16(temp);                       \
            (dest) = temp;                          \
        } while (0)

#define unpack_u32_into(dest)                       \
        do {                                        \
            unpack_u32(temp);                       \
            (dest) = temp;                          \
        } while (0)

#define unpack_u64_into(dest)                       \
        do {                                        \
            unpack_u64(temp);                       \
            (dest) = temp;                          \
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
            base += sizeof(value);                      \
        } while (0)

#define unpack_bytes_into(a, b)                                         \
                                do {                                    \
                if (!fail) {                                            \
                    (b) = unpack_bytes(end, base, &(a), "bytes");       \
                                                                        \
                    fail = (b) < 0;                                     \
                                                                        \
                    if (!fail) {                                        \
                        base += (b) + sizeof(int16_t);                  \
                    }                                                   \
                }                                                       \
            } while (0);

#define unpack_string_into(a)                                           \
            do {                                                        \
                if (!fail) {                                            \
                    (a)  = unpack_string(&base, end, &fail);            \
                }                                                       \
            } while (0)

    /*
    pxd_crypto_t *  crypto,
    char *          end,
    char *          base,
    char **         array,
    int *           array_size,
    const char *    name
    */
#define unpack_encrypted_into(a, b, c)                                  \
            do {                                                        \
                if (!fail) {                                            \
                    base = unpack_encrypted(crypto, end,                \
                        base, &a, &b, c);                               \
                    fail = base == null;                                \
                }                                                       \
            } while (0)

/*
 *  Define the string versions of the common error codes.  These
 *  values must match AnsCommon and PxdCommon.
 */
static const char *op_results[] =
        {
            "succeeded",
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
            "operation rejected",
            "connection lost",
            "credentials rejected",
            "quota exceeded"
        };

/*
 *  Configurable Parameters
 */
static int      pxd_max_packet_size    =  8192;
static int32_t  pxd_max_encrypt        =   256;
static int      pxd_partial_timeout    =    20;

/*
 *  Other values
 */
static int      pxd_login_version      =     1;
static int      pxd_challenge_version  =     1;

#ifdef pxd_print
static int      pxd_print_interval     =     0;
static int      pxd_print_count        =     0;
#endif

/*
 *  For testing
 */
static int      pxd_reject_count       =     0;
static int      pxd_partial_timeouts   =     0;
static int      pxd_write_limit        =     0;
static int      pxd_packets_out        =     0;
static int      pxd_short_packets      =     0;
static int      pxd_check_length       =     0;
static int      pxd_tag_errors         =     0;
static int      pxd_type_errors        =     0;
static int      pxd_length_mismatches  =     0;
static int      pxd_blob_signatures    =     0;
static int      pxd_demon_blob         =     0;
static int      pxd_packet_length      =     0;
static int      pxd_force_queue        =     0;
static int      pxd_packet_size_errors =     0;
static int      pxd_size_mismatches    =     0;
static int      pxd_fail_pack          = false;

const char *
pxd_response(uint64_t code)
{
    const char *  result;

    if (code < array_size(op_results)) {
        result = op_results[code];
    } else {
        result = "unknown";
    }

    return result;
}

/*
 *  Initialize a cryptography structure using the given key.
 */
pxd_crypto_t *
pxd_create_crypto(const void *key, int length, uint32_t seed)
{
    pxd_crypto_t *  crypto;

    int  aes_length;
    int  sha1_length;

    crypto = (pxd_crypto_t *) malloc(sizeof(*crypto));

    if (crypto == null) {
        return null;
    }

    memset(crypto, 0, sizeof(*crypto));

    sha1_length = pxd_min(length, sizeof(crypto->sha1_key));

    memcpy(crypto->sha1_key, key, sha1_length);

    /*
     *  Create a valid AES key.
     */
    memset(crypto->aes_key, 0, aes_block_size);
    aes_length = aes_block_size > length ? length : aes_block_size;
    memcpy(crypto->aes_key, crypto->sha1_key, aes_length);

    /*
     *  Initialize the random number generator.
     */
    mtwist_init(&crypto->mt, seed);

    return crypto;
}

void
pxd_free_crypto(pxd_crypto_t **crypto)
{
    if (*crypto == null) {
        return;
    }

    free(*crypto);
    *crypto = null;
}

pxd_command_t
pxd_find_command(pxd_io_t *io, pxd_unpacked_t *unpacked, int *found)
{
    int  i;

    pxd_command_t  result;

    memset(&result, 0, sizeof(result));
    *found = false;

    for (i = 0; i < array_size(io->outstanding) && !*found; i++) {
        *found =
                io->outstanding[i].valid
            &&  unpacked->async_id == io->outstanding[i].async_id;

        if (*found) {
            pxd_mutex_lock(&io->mutex);
            result = io->outstanding[i];
            io->outstanding[i].valid = false;
            pxd_mutex_unlock(&io->mutex);
        }
    }

    return result;
}

/*
 *  Encrypt a message via AES given an IV, a byte count, and a length.
 *  This routine does any needed padding to the cipher block length.
 */
int
pxd_encrypt
(
    pxd_crypto_t *  crypto,
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

    if (message_length <= 0 || message_length > pxd_max_encrypt) {
        return VPL_FALSE;
    }

    /*
     *  We are not using any of the auto-padding schemes, so compute
     *  the bytes necessary to round the plaintext (a two-byte count
     *  plus the message_in) up to the aes_block_size.  We will fill
     *  the padding area with zeros.  Also, compute the output size,
     *  which includes the IV.
     */
    padded_length  = sizeof(length) + message_length;

    pad_count      = aes_block_size - (padded_length % aes_block_size);
    pad_count     %= aes_block_size;

    padded_length += pad_count;
    output_length  = padded_length + aes_block_size;

    /*
     *  Allocate the buffers.
     */
    output         = (u8 *) malloc(output_length);
    padded_text    = (u8 *) malloc(padded_length);

    if (output == null || padded_text == null) {
        log_warn_1("malloc failure in encryption");
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
    result = aes_SwEncrypt(crypto->aes_key, (u8 *) iv, padded_text,
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
int
pxd_decrypt
(
    pxd_crypto_t *  crypto,
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
        log_error_1("malloc failed during decryption");
        return VPL_FALSE;
    }

    /*
     *  Decrypt the ciphertext.
     */
    result = aes_SwDecrypt(crypto->aes_key, iv, ciphertext,
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
        log_error_1("malloc failed during decryption (text)");
        return VPL_FALSE;
    }

    memcpy(text, work + 2, length);

    *plaintext    = (char *) text;
    *plain_length = length;

    free(work);
    return VPL_TRUE;
}

uint32_t
pxd_random(pxd_crypto_t *crypto)
{
    return mtwist_rand(&crypto->mt);
}

/*
 *  Encrypt an outgoing field given a count and a length.  This routine
 *  generates the IV.
 */
static int
encrypt_field
(
    pxd_crypto_t *  crypto,
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
    int      result;

    *encrypted = null;
    *encrypted_count = 0;

    iv = &buffer[0];
    memset(buffer, 0, sizeof(buffer));

    for (i = 0; i < array_size(random); i++) {
        random[i] = mtwist_rand(&crypto->mt);
    }

    memcpy(iv, random, pxd_min(sizeof(random), sizeof(buffer)));

    result = pxd_encrypt(crypto, encrypted, encrypted_count, iv, plaintext,
                plaintext_count);

    return result;
}

/*
 *  Create a packet structure.  Set the data and length fields,
 *  if given.
 */
pxd_packet_t *
pxd_alloc_packet(char *data, int length, void *opaque)
{
    pxd_packet_t *  packet;

    packet = (pxd_packet_t *) malloc(sizeof(*packet));

    if (packet == null) {
        return null;
    }

    memset(packet, 0, sizeof(*packet));

    packet->base     = data;
    packet->length   = length;
    packet->next     = null;
    packet->opaque   = opaque;

    return packet;
}

pxd_unpacked_t *
pxd_alloc_unpacked(void)
{
    pxd_unpacked_t *  unpacked;

    unpacked = (pxd_unpacked_t *) malloc(sizeof(*unpacked));

    if (unpacked == null) {
        return null;
    }

    memset(unpacked, 0, sizeof(*unpacked));

    unpacked->from_unpack = false;
    return unpacked;
}

static void
pxd_done(pxd_packet_t *packet, int sent)
{
    pxd_packet_cb_t  callback;

    if (packet->callback != null) {
        callback = packet->callback;
        packet->callback = null;

        callback(packet->io, packet, sent);
    }
}

/*
 *  Free a packet structure and any buffers attached to it.
 */
void
pxd_free_packet(pxd_packet_t **packet)
{
    if (*packet != null) {
        pxd_done(*packet, false);
        free((*packet)->base);
        free(*packet);
        *packet = null;
    }
}

int
pxd_is_signed(int type)
{
    int  result;

    result = true;

    switch (type) {
    case Send_pxd_challenge:
    case Send_ccd_challenge:
    case Reject_pxd_credentials:
    case Reject_ccd_credentials:
        result = false;
        break;
    }

    return result;
}

/*
 *  Convert a packet type to a string.
 */
const char *
pxd_packet_type(int type)
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

    case Set_device_compat:
        result = "Set_device_compat";
        break;

    case Send_state_compat:
        result = "Send_state_compat";
        break;

    case Reject_pxd_credentials:
        result = "Reject_pxd_credentials";
        break;

    case Send_pxd_login:
        result = "Send_pxd_login";
        break;

    case Send_ccd_login:
        result = "Send_ccd_login";
        break;

    case Reject_ccd_credentials:
        result = "Reject_ccd_credentials";
        break;

    case Send_ccd_challenge:
        result = "Send_ccd_challenge";
        break;

    case Send_pxd_challenge:
        result = "Send_pxd_challenge";
        break;

    case Declare_server:
        result = "Declare_server";
        break;

    case Query_server_declaration:
        result = "Query_server_declaration";
        break;

    case Send_server_declaration:
        result = "Send_server_declaration";
        break;

    case Start_connection_attempt:
        result = "Start_connection_attempt";
        break;

    case Start_proxy_connection:
        result = "Start_proxy_connection";
        break;

    case Send_pxd_response:
        result = "Send_pxd_response";
        break;

    case Send_ccd_response:
        result = "Send_ccd_response";
        break;

    case Set_pxd_configuration:
        result = "Set_pxd_configuration";
        break;

    default:
        result = "unknown";
        break;
    }

    return result;
}

#define packed_string_length(x) (sizeof(uint16_t) + strlen(x))
#define packed_bytes_length(x)  (sizeof(uint16_t) + (x))

static int
packed_address_length(pxd_address_t *address)
{
    int  result;

    result  = packed_bytes_length(address->ip_length);
    result += sizeof(uint32_t);
    result += sizeof(char);

    return result;
}

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
append_int(char **basep, char *end, uint32_t value)
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

static int
append_bytes(char **basep, char *end, const char *bytes, int length)
{
    uint16_t  wire_length;
    char *    base;

    base = *basep;

    if (end - base < sizeof(wire_length) + length) {
        return true;
    }

    wire_length = VPLConv_ntoh_u16(length);

    memcpy(base, &wire_length, sizeof(wire_length));
    base += sizeof(wire_length);

    memcpy(base, bytes, length);

    base   += length;
    *basep  = base;
    return false;
}

static int
append_address(char **basep, char *end, pxd_address_t *address)
{
    int  fail;

    fail  = append_bytes(basep, end, (char *) address->ip_address, address->ip_length);
    fail |= append_int  (basep, end, address->port);
    fail |= append_byte (basep, end, address->type);
    return fail;
}

static int
addresses_length(pxd_unpacked_t *unpacked)
{
    int  req_length;

    req_length = 0;

    for (int i = 0; i < unpacked->address_count; i++) {
        req_length += packed_address_length(&unpacked->addresses[i]);
    }

    return req_length;
}

/*
 *  Create a packet in wire format.
 */
pxd_packet_t *
pxd_pack(pxd_unpacked_t *unpacked, void *opaque, pxd_error_t *error)
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
    uint16_t    param_count;
    uint64_t    user_id;
    uint64_t    device_id;
    uint64_t    sequence_id;
    uint64_t    tag;
    uint64_t    async_id;
    int         i;
    int         fail;
    int         count;
    short       ioac;
    int         type_length;
    int         app_length;
    int         is_signed;

    pxd_packet_t *    packet;
    char *            end;

    clear_error(error);

    req_length       = header_size;
    user_id          = VPLConv_ntoh_u64(unpacked->user_id);
    device_id        = VPLConv_ntoh_u64(unpacked->device_id);
    data             = unpacked->data;
    tag              = VPLConv_ntoh_u64(unpacked->connection_tag);
    sequence_id      = VPLConv_ntoh_u64(unpacked->out_sequence);
    encrypted        = null;
    ioac             = unpacked->ioac_type;
    count            = unpacked->count;
    type_length      = 0;
    app_length       = 0;
    encrypted_count  = 0;
    version          = 1;
    fail             = false;

    /*
     *  Compute the total packet length.
     */
    switch (unpacked->type) {
    case Send_response:
        req_length += sizeof(int64_t);
        break;

    case Send_pxd_challenge:
    case Send_ccd_challenge:
        unpacked->version = pxd_challenge_version;

        req_length += sizeof(uint16_t);
        req_length += sizeof(int64_t);
        req_length += packed_bytes_length(unpacked->challenge_length);
        req_length += packed_string_length(unpacked->pxd_dns);
        req_length += sizeof(int64_t);
        req_length += packed_bytes_length(unpacked->address_length);
        req_length += sizeof(int32_t);
        break;

    case Send_pxd_login:
    case Send_ccd_login:
        req_length += sizeof(uint16_t);
        req_length += sizeof(int64_t);
        req_length += packed_bytes_length(unpacked->challenge_length);
        req_length += packed_bytes_length(unpacked->blob_length);

        if (unpacked->type == Send_pxd_login) {
            req_length += packed_string_length(unpacked->pxd_dns);
        }

        break;

    case Reject_pxd_credentials:
    case Reject_ccd_credentials:
        break;

    case Declare_server:
        req_length += sizeof(uint16_t);
        req_length += packed_string_length(unpacked->region);
        req_length += 2 * sizeof(uint64_t); // user id, device id
        req_length += packed_string_length(unpacked->instance_id);
        req_length += packed_string_length(unpacked->ans_dns);
        req_length += packed_string_length(unpacked->pxd_dns);
        req_length += sizeof(uint16_t);
        req_length += addresses_length(unpacked);
        break;

    case Query_server_declaration:
        req_length += sizeof(int16_t);
        req_length += packed_string_length(unpacked->region);
        req_length += 2 * sizeof(int64_t);
        req_length += packed_string_length(unpacked->instance_id);
        break;

    case Send_server_declaration:
        req_length += sizeof(uint16_t);
        req_length += packed_string_length(unpacked->region);
        req_length += sizeof(int64_t);
        req_length += sizeof(int64_t);
        req_length += packed_string_length(unpacked->instance_id);
        req_length += packed_string_length(unpacked->ans_dns);
        req_length += packed_string_length(unpacked->pxd_dns);
        req_length += sizeof(int16_t);

        for (int i = 0; i < unpacked->address_count; i++) {
            req_length += packed_address_length(&unpacked->addresses[i]);
        }

        break;

    case Start_connection_attempt:
        req_length += sizeof(int16_t);
        req_length += packed_string_length(unpacked->region);
        req_length += sizeof(int64_t);
        req_length += sizeof(int64_t);
        req_length += packed_string_length(unpacked->instance_id);
        req_length += packed_string_length(unpacked->pxd_dns);
        req_length += sizeof(int64_t);
        req_length += sizeof(int16_t);

        for (int i = 0; i < unpacked->address_count; i++) {
            req_length += packed_address_length(&unpacked->addresses[i]);
        }

        break;

    case Start_proxy_connection:
        req_length +=     sizeof(uint16_t);
        req_length += 3 * sizeof(int64_t);
        req_length += packed_string_length(unpacked->instance_id);
        break;

    case Send_pxd_response:
        req_length += sizeof(int16_t);
        req_length += sizeof(int64_t);
        break;

    case Send_ccd_response:
        req_length += sizeof(int16_t);
        req_length += sizeof(int64_t);
        req_length += sizeof(char);

        if (unpacked->address != null) {
            req_length += packed_bytes_length(unpacked->address_length);
            req_length += sizeof(uint32_t);
        }

        break;

    case Set_pxd_configuration:
        req_length += 2 * sizeof(uint16_t);
        req_length += pxd_config_params * sizeof(uint32_t);

        if (unpacked->extra != 0) {
            req_length += sizeof(uint32_t);
        }

        break;

    default:
        log_error("pack() got an unexpected packet type %d", unpacked->type);
        error->error   = VPL_ERR_INVALID;
        error->message = "The packet type is invalid";
        return null;
    }

    is_signed = pxd_is_signed(unpacked->type);

    if (is_signed) {
        req_length += signature_size;
    }

    if (req_length > pxd_max_packet_size) {
        log_error("That packet size (%d) is too large - type %s (%d)",
            (int) req_length,
                  pxd_packet_type(unpacked->type),
            (int) unpacked->type);
        error->error   = VPL_ERR_INVALID;
        error->message = "The packet is too large.";
        return null;
    }

    body = (char *) malloc(req_length);

    if (body == null) {
        log_error("malloc failed in pxd_pack (body, %s, %d)",
            pxd_packet_type(unpacked->type), (int) unpacked->type);
        error->error   = VPL_ERR_NOMEM;
        error->message = "malloc failed for the output buffer";
        return null;
    }

    end = body + req_length;

    /*
     *  Okay, build the header.  The true sequence number is inserted later.
     */
    memset(body, 0, req_length);
    base = body;

    length = VPLConv_ntoh_u16(~req_length);
    memcpy(base, &length, sizeof(length));
    base += sizeof(length);

    length = VPLConv_ntoh_u16(req_length);
    memcpy(base, &length, sizeof(length));
    base += sizeof(length);

    type = VPLConv_ntoh_u16(unpacked->type);
    memcpy(base, &type, sizeof(type));
    base += sizeof(type);

    memcpy(base, &tag, sizeof(tag));
    base += sizeof(user_id);

    async_id = VPLConv_ntoh_u64(unpacked->async_id);
    memcpy(base, &async_id, sizeof(async_id));
    base += sizeof(async_id);

    memcpy(base, &sequence_id, sizeof(sequence_id));
    base += sizeof(sequence_id);

    /*
     *  Now append the variable fields.
     */
    switch (unpacked->type) {
    case Send_response:
        fail |= append_long(&base, end, unpacked->response);
        break;

    case Reject_pxd_credentials:
    case Reject_ccd_credentials:
        break;

    case Send_pxd_login:
    case Send_ccd_login:
        unpacked->version = pxd_login_version;

        fail |= append_short(&base, end, unpacked->version);
        fail |= append_long (&base, end, unpacked->connection_id);
        fail |= append_bytes(&base, end, unpacked->challenge, unpacked->challenge_length);
        fail |= append_bytes(&base, end, unpacked->blob,      unpacked->blob_length);

        if (unpacked->type == Send_pxd_login) {
            fail |= append_string(&base, end, unpacked->pxd_dns);
        }

        break;

    case Send_pxd_challenge:
    case Send_ccd_challenge:
        fail |= append_short (&base, end, unpacked->version);
        fail |= append_long  (&base, end, unpacked->connection_id);
        fail |= append_bytes (&base, end, unpacked->challenge, unpacked->challenge_length);
        fail |= append_string(&base, end, unpacked->pxd_dns);
        fail |= append_long  (&base, end, unpacked->connection_time);
        fail |= append_bytes (&base, end, unpacked->address,   unpacked->address_length);
        fail |= append_int   (&base, end, unpacked->port);
        break;

    case Declare_server:
        fail |= append_short (&base, end, unpacked->version);
        fail |= append_string(&base, end, unpacked->region);
        fail |= append_long  (&base, end, unpacked->user_id);
        fail |= append_long  (&base, end, unpacked->device_id);
        fail |= append_string(&base, end, unpacked->instance_id);
        fail |= append_string(&base, end, unpacked->ans_dns);
        fail |= append_string(&base, end, unpacked->pxd_dns);
        fail |= append_short (&base, end, unpacked->address_count);

        for (i = 0; i < unpacked->address_count; i++) {
            append_address(&base, end, &unpacked->addresses[i]);
        }

        break;

    case Query_server_declaration:
        fail |= append_short (&base, end, unpacked->version);
        fail |= append_string(&base, end, unpacked->region);
        fail |= append_long  (&base, end, unpacked->user_id);
        fail |= append_long  (&base, end, unpacked->device_id);
        fail |= append_string(&base, end, unpacked->instance_id);
        break;

    case Send_server_declaration:
        fail |= append_short (&base, end, unpacked->version);
        fail |= append_string(&base, end, unpacked->region);
        fail |= append_long  (&base, end, unpacked->user_id);
        fail |= append_long  (&base, end, unpacked->device_id);
        fail |= append_string(&base, end, unpacked->instance_id);
        fail |= append_string(&base, end, unpacked->ans_dns);
        fail |= append_string(&base, end, unpacked->pxd_dns);
        fail |= append_short (&base, end, unpacked->address_count);

        for (i = 0; i < unpacked->address_count; i++) {
            fail |= append_address(&base, end, &unpacked->addresses[i]);
        }

        break;

    case Start_connection_attempt:
        fail |= append_short (&base, end, version                );
        fail |= append_string(&base, end, unpacked->region       );
        fail |= append_long  (&base, end, unpacked->user_id      );
        fail |= append_long  (&base, end, unpacked->device_id    );
        fail |= append_string(&base, end, unpacked->instance_id  );
        fail |= append_string(&base, end, unpacked->pxd_dns      );
        fail |= append_long  (&base, end, unpacked->request_id   );
        fail |= append_short (&base, end, unpacked->address_count);

        for (i = 0; i < unpacked->address_count; i++) {
            fail |= append_address(&base, end, &unpacked->addresses[i]);
        }

        break;

    case Start_proxy_connection:
        fail |= append_short (&base, end, version              );
        fail |= append_long  (&base, end, unpacked->user_id    );
        fail |= append_long  (&base, end, unpacked->device_id  );
        fail |= append_string(&base, end, unpacked->instance_id);
        fail |= append_long  (&base, end, unpacked->request_id );
        break;

    case Send_pxd_response:
        fail |= append_short (&base, end, version);
        fail |= append_long  (&base, end, unpacked->response);
        break;

    case Send_ccd_response:
        fail |= append_short (&base, end, version);
        fail |= append_long  (&base, end, unpacked->response);
        fail |= append_byte  (&base, end, unpacked->address != null);

        if (unpacked->address != null) {
            fail |= append_bytes(&base, end, unpacked->address, unpacked->address_length);
            fail |= append_int  (&base, end, unpacked->port);
        }

        break;

    case Set_pxd_configuration:
        param_count = pxd_config_params;

        if (unpacked->extra != 0) {
            param_count++;
        }

        fail |= append_short (&base, end, version);
        fail |= append_short (&base, end, param_count);

        fail |= append_int   (&base, end, unpacked->proxy_retries  );
        fail |= append_int   (&base, end, unpacked->proxy_wait     );
        fail |= append_int   (&base, end, unpacked->idle_limit     );
        fail |= append_int   (&base, end, unpacked->sync_io_timeout);
        fail |= append_int   (&base, end, unpacked->min_delay      );
        fail |= append_int   (&base, end, unpacked->max_delay      );
        fail |= append_int   (&base, end, unpacked->thread_retries );
        fail |= append_int   (&base, end, unpacked->max_packet_size);
        fail |= append_int   (&base, end, unpacked->max_encrypt    );
        fail |= append_int   (&base, end, unpacked->partial_timeout);
        fail |= append_int   (&base, end, unpacked->reject_limit   );

        if (unpacked->extra != 0) {
            fail |= append_int   (&base, end, unpacked->extra);
        }

        break;
    }

    /*
     *  Add the signature size to our offset.  The signature itself is
     *  inserted later.
     */
    if (is_signed) {
        fail |= end - base < signature_size;

        if (!fail) {
            memset(base, 0, signature_size);
            base += signature_size;
        }
    }

    /*
     *  Check that the packet is properly formed.
     */
    if (base - body != req_length || fail || pxd_fail_pack) {
        free(body);
        log_error("The packet size (type %s, %d) doesn't match the expected size.",
            pxd_packet_type(unpacked->type), (int) unpacked->type);
        error->error   = VPL_ERR_INVALID;
        error->message = "I made a size error";
        return null;
    }

    /*
     *  Okay, we got this far, so allocate the control data for the packet.
     */
    packet = pxd_alloc_packet(body, req_length, opaque);

    if (packet == null) {
        log_error("The packet malloc failed (%s, %d).",
            pxd_packet_type(unpacked->type), (int) unpacked->type);
        free(body);
        error->error   = VPL_ERR_NOMEM;
        error->message = "malloc failed in pxd_pack (packet)";
        return null;
    }

    log_info("created a packet, type %s (%d), tag " FMTu64 ", op id " FMTu64,
              pxd_packet_type(unpacked->type),
        (int) unpacked->type,
              unpacked->connection_tag,
              unpacked->async_id);

    packet->type     = unpacked->type;
    packet->async_id = unpacked->async_id;
    return packet;
}

void
pxd_sign_packet(pxd_crypto_t *crypto, char *packet, int length)
{
    CSL_ShaContext  context;

    CSL_ResetSha (&context);
    CSL_InputSha (&context, crypto->sha1_key, sizeof(crypto->sha1_key));
    CSL_InputSha (&context, (unsigned char *) packet,  length - signature_size);
    CSL_ResultSha(&context, (unsigned char *) packet + length - signature_size);
}

int
pxd_packet_limit(void)
{
    return pxd_max_packet_size;
}

void
pxd_prep_sync(pxd_packet_t *packet, int64_t *sequence, pxd_crypto_t *crypto)
{
    u8 *     buffer;
    u8 *     signature_area;
    int64_t  sequence_id;

    CSL_ShaContext    context;

    sequence_id = (*sequence)++;
    sequence_id = VPLConv_ntoh_u64(sequence_id);

    memcpy(packet->base + sequence_offset, &sequence_id, sizeof(sequence_id));

    if (pxd_is_signed(packet->type)) {
        buffer  = (unsigned char *) packet->base;

        signature_area = buffer + packet->length - signature_size;

        CSL_ResetSha (&context);
        CSL_InputSha (&context, crypto->sha1_key, sizeof(crypto->sha1_key));
        CSL_InputSha (&context, buffer, packet->length - signature_size);
        CSL_ResultSha(&context, signature_area);
    }

    packet->buffer = packet->base;
}

/*
 *  Add the sequence number to a packet and sign it.  We also increment the
 *  outgoing sequence id here.
 */
void
pxd_prep_packet(pxd_io_t *io, pxd_packet_t *packet, int64_t *sequence)
{
    uint64_t          sequence_id;
    uint64_t          tag;
    CSL_ShaContext    context;
    unsigned char *   buffer;
    unsigned char *   signature_area;
    int               verbose;
    pxd_crypto_t *    crypto;

    verbose = packet->verbose;
    crypto  = io->crypto;

    if (packet->prepared) {
        return;
    }

    packet->prepared = true;
    packet->buffer   = packet->base;

    /*
     *  Insert the correct sequence id.
     */
    packet->sequence_id = *sequence;    // used this for debugging
    sequence_id = VPLConv_ntoh_u64(*sequence);
    (*sequence)++;

    memcpy(packet->buffer + sequence_offset, &sequence_id, sizeof(sequence_id));

    /*
     *  Now fix the tag.
     */
    tag = VPLConv_ntoh_u64(io->tag);
    packet->tag = io->tag;

    memcpy(packet->buffer + tag_offset, &tag, sizeof(tag));

#if 0
    if (packet->type != Send_timed_ping) {
        log_info("assign packet %p (%s, %d) tag " FMTs64 ", sequence " FMTu64,
            packet,
            pxd_packet_type(packet->type),
            packet->type,
            packet->tag,
            packet->sequence_id);
    }
#endif

    if (pxd_is_signed(packet->type)) {
        buffer  = (unsigned char *) packet->buffer;

        signature_area = buffer + packet->length - signature_size;

        CSL_ResetSha (&context);
        CSL_InputSha (&context, crypto->sha1_key, sizeof(crypto->sha1_key));
        CSL_InputSha (&context, buffer, packet->length - signature_size);
        CSL_ResultSha(&context, signature_area);
    }
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
        log_error("The packet has no room for the %s field (length)", name);
        return -1;
    }

    fail = false;
    unpack_u16(length);

    if (fail || end - base < length) {
        log_error("The packet has no room for the %s field", name);
        return -1;
    }

    *array = base;
    return length;
}

static char *
unpack_string(char **base, char *end, int *error)
{
    char *  body;
    int     length;
    char *  result;

    if (*error) {
        return null;
    }

    length = unpack_bytes(end, *base, &body, "unpack_string");

    if (length < 0) {
        *error = true;
        return null;
    }

    result = (char *) malloc(length + 1);

    if (result == null) {
        log_warn_1("unpack_string failed in malloc");
        *error = true;
        return null;
    }

    memcpy(result, body, length);
    result[length] = 0;

    *base = *base + packed_bytes_length(length);
    return result;
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
    pxd_crypto_t *  crypto,
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

    *array       = null;
    *array_size  = 0;

    ciphertext_count = unpack_bytes(end, base, &ciphertext, name);

    if (ciphertext_count <= 0) {
        log_error("pxd_decrypt failed on the %s field, length %d", name, ciphertext_count);
        return null;
    }

    success = pxd_decrypt(crypto, array,
                 array_size, ciphertext, ciphertext_count);

    if (!success) {
        log_error("pxd_decrypt failed on the %s field", name);
        return null;
    }

    return ciphertext + ciphertext_count;
}

#define device_state(response)  \
            ((response) > max_device_state ? device_sleeping : (response))

void
pxd_free_unpacked(pxd_unpacked_t **unpackedp)
{
    pxd_unpacked_t *  unpacked;

    unpacked = *unpackedp;

    if (unpacked == null) {
        return;
    }

    if (unpacked->from_unpack) {
        free(unpacked->data);
        free(unpacked->host_name);
        free(unpacked->device_list);
        free(unpacked->device_states);
        free(unpacked->device_times);
        free(unpacked->notification);
        free(unpacked->mac_address);
        free(unpacked->sleep_packet);
        free(unpacked->sleep_dns);
        free(unpacked->wakeup_key);
        free(unpacked->instance_id);
        free(unpacked->addresses);
        free(unpacked->region);
        free(unpacked->ans_dns);
        free(unpacked->pxd_dns);
        free(unpacked->sleep_dns);
        free(unpacked->server_instance);

        pxd_free_blob  (&unpacked->unpacked_blob);
        pxd_free_packet(&unpacked->packet);
    }

    memset(unpacked,0, sizeof(*unpacked));
    free(unpacked);

    *unpackedp = null;
}

static void
unpack_addresses
(
    int count, char **basep, char *end, pxd_unpacked_t *result, int *error
)
{
    char *  base;
    int     i;
    int     length;
    int     fail;

    if (*error) {
        return;
    }

    result->addresses = (pxd_address_t *) malloc(count * sizeof(pxd_address_t));

    if (result->addresses == null) {
        log_warn_1("unpack_addresses failed in malloc");
        *error = true;
        return;
    }

    memset(result->addresses, 0, count * sizeof(pxd_address_t));
    base = *basep;

    for (i = 0; i < count && !*error; i++) {
        length =
            unpack_bytes
            (
                end, base, &result->addresses[i].ip_address, "declare ip"
            );

        if (length < 0 || end - base < sizeof(uint32_t) + sizeof(char)) {
            *error = true;
        } else {
            fail = false;
            base += packed_bytes_length(length);
            result->addresses[i].ip_length = length;
            unpack_u32_into(result->addresses[i].port);
            result->addresses[i].type = *base;
            base++;
        }
    }

    if (*error) {
        free(result->addresses);
        result->addresses = null;
    }

    *basep = base;
}

int
pxd_get_type(pxd_packet_t *packet)
{
    char *  base;
    char *  end;
    int     fail;

    base = packet->base;
    end  = base + packet->length;
    fail = false;

    /*
     *  Unpack the header.  This code is duplicated in pxd_unpack().
     */
    unpack_u16(check_length);
    unpack_u16(length);

    if (fail || check_length != (uint16_t) ~length) {
        log_warn_1("The check length doesn't match the length.");
        return -1;
    }

    unpack_u16(type);

    return fail ? -1 : type;
}

/*
 *  Validate a signature on a packet.
 */
int
pxd_check_signature(pxd_crypto_t *crypto, pxd_packet_t *packet)
{
    unsigned char *   received;
    unsigned char *   buffer;
    unsigned char     expected[CSL_SHA1_DIGESTSIZE];
    unsigned int      length;
    CSL_ShaContext    context;
    int               type;

    /*
     *  Credential rejection packets et al aren't signed.  Check carefully
     *  for such a packet.
     */
    type = pxd_get_type(packet);

    if (type < 0) {
        return false;
    }

    if (!pxd_is_signed(type)) {
        return true;
    }

    if (packet->length < header_size + signature_size) {
        return false;
    }

    length  = signature_size;
    buffer  = (unsigned char *) packet->base;

    /*
     *  Compute the signature.
     */
    CSL_ResetSha (&context);
    CSL_InputSha (&context, crypto->sha1_key, sizeof(crypto->sha1_key));
    CSL_InputSha (&context, buffer, packet->length - signature_size);
    CSL_ResultSha(&context, expected);

    /*
     *  Compute the address of the signature inside the packet.
     */
    received = buffer + packet->length - signature_size;

    /*
     *  Check the expected signature and what we got.
     */
    if (memcmp(expected, received, length) != 0) {
        return VPL_FALSE;
    }

    return VPL_TRUE;
}

/*
 *  Unpack a packet from wire format.  The caller should
 *  validate the signature, if there is one.
 */
pxd_unpacked_t *
pxd_unpack(pxd_crypto_t *crypto, pxd_packet_t *packet, int64_t *expected_id, uint64_t tag)
{
    int       remaining;
    uint64_t  connection_tag;
    char *    base;
    char *    end;
    int       fail;
    uint16_t  address_count;
    int       is_signed;
    int       config_count;
    int       address_present;

    pxd_unpacked_t *  result;

    /*
     *  Create a control structure for the packet.
     */
    result = pxd_alloc_unpacked();

    if (result == null) {
        log_error_1("malloc failed during unpack.");
        return null;
    }

    result->from_unpack = true;

    /*
     *  Point the control structure at the actual data from the network.
     */
    base = packet->base;
    end  = base + packet->length;
    fail = false;

    /*
     *  Make sure that it's long enough to be valid.
     */
    if (packet->length < header_size) {
        pxd_short_packets++;
        log_error("I got a bad packet:  "
            "packet->length (%d) < header_size (%d)",
            packet->length, header_size);
        pxd_free_unpacked(&result);
        return null;
    }

    /*
     *  Unpack the header.
     */
    unpack_u16(check_length);
    unpack_u16(length);

    if ((uint16_t) ~check_length != length) {
        pxd_check_length++;
        log_error_1("unpack got a check size mismatch");
        pxd_free_unpacked(&result);
        return null;
    }

    if (packet->length != length) {
        pxd_length_mismatches++;
        log_error_1("unpack got a length mismatch");
        pxd_free_unpacked(&result);
        return null;
    }

    unpack_u16(type);
    unpack_u64(user_id);    // for pxd, this is the connection tag
    unpack_u64(async_id);
    unpack_u64(sequence_id);

    connection_tag = user_id;

    /*
     *  Set the fields we pass to the user.
     */
    result->type            = type;
    result->user_id         = user_id;
    result->connection_tag  = user_id;       // pxd has a connection tag.
    result->async_id        = async_id;
    result->out_sequence    = sequence_id;

    /*
     *  Check that the sequence number is correct, if this packet
     *  type has a sequence number.  If the sequence number is okay,
     *  increment the counter.
     */
    if (expected_id != null) {
        if (sequence_id != *expected_id) {
            log_error("I was expecting sequence number " FMTu64
                ", but got " FMTu64 " (type %s).", *expected_id, sequence_id,
                pxd_packet_type(type));
            pxd_free_unpacked(&result);
            return null;
        }

        (*expected_id)++;
    }

    /*
     *  Now check the connection tag.
     */
    if (connection_tag != tag) {
        pxd_tag_errors++;
        log_error("I got connection tag " FMTu64 ", expected " FMTu64 " for %s (%d)",
            connection_tag, tag, pxd_packet_type(type), (int) type);
        pxd_free_unpacked(&result);
        return null;
    }

    /*
     *  Check whether the type is one that we can decode.
     */
    switch (type) {
    case Send_unicast:
    case Send_multicast:
    case Send_challenge:
    case Set_device_params:
    case Send_ping:
    case Reject_credentials:
    case Send_state_list:
    case Send_sleep_setup:
    case Send_device_update:
    case Send_response:
    case Set_device_compat:
    case Send_state_compat:
    case Send_timed_ping:
    case Reject_pxd_credentials:
    case Reject_ccd_credentials:
    case Send_pxd_login:
    case Send_ccd_login:
    case Send_pxd_challenge:
    case Send_ccd_challenge:
    case Declare_server:
    case Query_server_declaration:
    case Send_server_declaration:
    case Start_connection_attempt:
    case Start_proxy_connection:
    case Send_pxd_response:
    case Send_ccd_response:
    case Set_pxd_configuration:
        break;

    default:
        pxd_type_errors++;
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
    case Send_response:
        unpack_u64_into(result->response);
        log_info("op id " FMTu64 " returned \"%s\" (" FMTu64 ")",
            result->async_id, pxd_response(result->response), result->response);
        break;

    case Reject_pxd_credentials:
    case Reject_ccd_credentials:
        break;

    case Send_pxd_login:
    case Send_ccd_login:
        unpack_u16_into  (result->version                             );
        unpack_u64_into  (result->connection_id                       );
        unpack_bytes_into(result->challenge,  result->challenge_length);
        unpack_bytes_into(result->blob,       result->blob_length     );

        if (type == Send_pxd_login) {
            unpack_string_into(result->pxd_dns);
        }

        break;

    case Send_pxd_challenge:
    case Send_ccd_challenge:
        unpack_u16_into   (result->version                             );
        unpack_u64_into   (result->connection_id                       );
        unpack_bytes_into (result->challenge,  result->challenge_length);
        unpack_string_into(result->pxd_dns                             );
        unpack_u64_into   (result->connection_time                     );
        unpack_bytes_into (result->address,    result->address_length  );
        unpack_u32_into   (result->port                                );
        break;

    case Declare_server:
        unpack_u16_into   (result->version    );
        unpack_string_into(result->region     );
        unpack_u64_into   (result->user_id    );
        unpack_u64_into   (result->device_id  );
        unpack_string_into(result->instance_id);
        unpack_string_into(result->ans_dns    );
        unpack_string_into(result->pxd_dns    );
        unpack_u16_into   (address_count      );

        unpack_addresses(address_count, &base, end, result, &fail);
        result->address_count = address_count;
        break;

    case Query_server_declaration:
        unpack_u16_into   (result->version    );
        unpack_string_into(result->region     );
        unpack_u64_into   (result->user_id    );
        unpack_u64_into   (result->device_id  );
        unpack_string_into(result->instance_id);
        break;

    case Send_server_declaration:
        unpack_u16_into   (result->version    );
        unpack_string_into(result->region     );
        unpack_u64_into   (result->user_id    );
        unpack_u64_into   (result->device_id  );
        unpack_string_into(result->instance_id);
        unpack_string_into(result->ans_dns    );
        unpack_string_into(result->pxd_dns    );
        unpack_u16_into   (result->address_count);
        unpack_addresses  (result->address_count, &base, end, result, &fail);
        break;

    case Start_connection_attempt:
        unpack_u16_into   (result->version      );
        unpack_string_into(result->region       );
        unpack_u64_into   (result->user_id      );
        unpack_u64_into   (result->device_id    );
        unpack_string_into(result->instance_id  );
        unpack_string_into(result->pxd_dns      );
        unpack_u64_into   (result->request_id   );
        unpack_u16_into   (result->address_count);

        unpack_addresses  (result->address_count, &base, end, result, &fail);
        break;

    case Start_proxy_connection:
        unpack_u16_into   (result->version    );
        unpack_u64_into   (result->user_id    );
        unpack_u64_into   (result->device_id  );
        unpack_string_into(result->instance_id);
        unpack_u64_into   (result->request_id );
        break;

    case Send_pxd_response:
        unpack_u16_into   (result->version    );
        unpack_u64_into   (result->response   );
        break;

    case Send_ccd_response:
        unpack_u16_into   (result->version    );
        unpack_u64_into   (result->response   );
        unpack_u8_into    (address_present    );

        if (address_present) {
            unpack_bytes_into(result->address,  result->address_length);
            unpack_u32_into  (result->port);
        }

        break;

    case Set_pxd_configuration:
        unpack_u16_into   (result->version        );
        unpack_u16_into   (config_count           );
        unpack_u32_into   (result->proxy_retries  );
        unpack_u32_into   (result->proxy_wait     );
        unpack_u32_into   (result->idle_limit     );
        unpack_u32_into   (result->sync_io_timeout);
        unpack_u32_into   (result->min_delay      );
        unpack_u32_into   (result->max_delay      );
        unpack_u32_into   (result->thread_retries );
        unpack_u32_into   (result->max_packet_size);
        unpack_u32_into   (result->max_encrypt    );
        unpack_u32_into   (result->partial_timeout);

        config_count -= 10;

        if (config_count > 0) {
            unpack_u32_into   (result->reject_limit   );
            config_count--;
        }

        while (config_count-- > 0) {
            unpack_u32_into(result->extra);
        }

        break;
    }

    /*
     *  Check that all that remains in the packet is the signature, if there
     *  is a signature.
     */
    is_signed = pxd_is_signed(type);

    if (is_signed) {
        remaining = end - base - signature_size;
    } else {
        remaining = end - base;
    }

    /*
     *  Now see whether the size meets the expectations.
     */
    if (fail || remaining != 0) {
        pxd_packet_length++;
        pxd_free_unpacked(&result);
        log_error("The packet (%s, %d) is corrupt - length %d"
            " - version mismatch?",
                  pxd_packet_type(type),
            (int) type,
            (int) packet->length);
        return null;
    }

    /*
     *  Count the rejections received.
     */
    switch (type) {
    case Reject_credentials:
    case Reject_pxd_credentials:
    case Reject_ccd_credentials:
        pxd_reject_count++;
        break;

    default:
        break;
    }

    if (packet->verbose) {
        log_info("finished unpacking %s (%d), op id " FMTs64,
                  pxd_packet_type(result->type),
            (int) result->type,
                  packet->async_id);
    }

    return result;
}

char *
pxd_pack_ans(pxd_unpacked_t *unpacked, int *length_out)
{
    char *  buffer;
    char *  base;
    int     length;
    char *  end;
    int     fail;

    length  = sizeof(int16_t);          /* type */

    switch (unpacked->type) {
    case pxd_connect_request:
        length += 3 * sizeof(uint64_t);     /* user, device, request ids */
        length += packed_string_length(unpacked->instance_id    );
        length += packed_string_length(unpacked->pxd_dns        );
        length += packed_string_length(unpacked->server_instance);
        length += sizeof(int16_t);          /* address count */
        length += addresses_length(unpacked);
        break;

    case pxd_wakeup:
        length += packed_string_length(unpacked->instance_id);
        break;

    default:
        return null;
    }

    buffer = base = (char *) malloc(length);

    if (base == null) {
        return null;
    }

    end  = base + length;
    fail = append_short(&base, end, unpacked->type);

    switch (unpacked->type) {
    case pxd_connect_request:
        fail |= append_long  (&base, end, unpacked->user_id        );
        fail |= append_long  (&base, end, unpacked->device_id      );
        fail |= append_string(&base, end, unpacked->instance_id    );
        fail |= append_long  (&base, end, unpacked->request_id     );
        fail |= append_string(&base, end, unpacked->pxd_dns        );
        fail |= append_string(&base, end, unpacked->server_instance);
        fail |= append_short (&base, end, unpacked->address_count  );

        for (int i = 0; i < unpacked->address_count; i++) {
            fail |= append_address(&base, end, &unpacked->addresses[i]);
        }

        break;

    case pxd_wakeup:
        fail |= append_string(&base, end, unpacked->instance_id  );
        break;
    }

    if (fail || base != end || pxd_fail_pack) {
        log_error_1("pxd_pack_ans failed");
        free(buffer);
        buffer = null;
    }

    *length_out = length;
    return buffer;
}

pxd_unpacked_t *
pxd_unpack_ans(char *buffer, int length, pxd_error_t *error)
{
    int     fail;
    char *  base;
    char *  end;

    pxd_unpacked_t *  unpacked;

    clear_error(error);

    unpacked = pxd_alloc_unpacked();

    if (unpacked == null) {
        error->error    = VPL_ERR_NOMEM;
        error->message  = "malloc failed in pxd_unpack_ans";
        pxd_free_unpacked(&unpacked);
        return null;
    }

    unpacked->from_unpack = true;

    fail = false;
    base = buffer;
    end  = buffer + length;

    unpack_u16_into(unpacked->type);

    switch (unpacked->type) {
    case pxd_connect_request:
        unpack_u64_into   (unpacked->user_id        );
        unpack_u64_into   (unpacked->device_id      );
        unpack_string_into(unpacked->instance_id    );
        unpack_u64_into   (unpacked->request_id     );
        unpack_string_into(unpacked->pxd_dns        );
        unpack_string_into(unpacked->server_instance);
        unpack_u16_into   (unpacked->address_count  );

        unpack_addresses  (unpacked->address_count, &base, end, unpacked, &fail);
        break;

    case pxd_wakeup:
        unpack_string_into(unpacked->instance_id  );
        break;

    default:
        fail = true;
        break;
    }

    if (fail || end != base) {
        pxd_free_unpacked(&unpacked);
        error->error   = VPL_ERR_INVALID;
        error->message = "The ANS message buffer was corrupt";
    }

    return unpacked;
}

void
pxd_free_blob(pxd_blob_t **blobp)
{
    pxd_blob_t *  blob;

    blob = *blobp;

    if (blob == null) {
        return;
    }

    free(blob->key            );
    free(blob->handle         );
    free(blob->service_id     );
    free(blob->ticket         );
    free(blob->client_instance);
    free(blob->server_instance);

    free(blob);
    *blobp = null;
    return;
}

char *
pxd_pack_blob
(
    int *           length_out,
    pxd_blob_t *    blob,
    pxd_crypto_t *  crypto,
    pxd_error_t *   error
)
{
    int     pass;
    int     fail;
    char *  result;
    char *  base;
    char *  end;
    int     length;
    char *  key;
    int     key_length;
    char *  handle;
    int     handle_length;
    char *  sid;
    int     sid_length;
    char *  ticket;
    int     ticket_length;
    char    signature[signature_size];
    int16_t count;
    int16_t fields;
    int16_t version;
    int     signed_length;

    CSL_ShaContext  context;

    *length_out = 0;

    pass  = encrypt_field(crypto, &key, &key_length,
                blob->key, blob->key_length);

    pass &= encrypt_field(crypto, &handle, &handle_length,
                blob->handle, blob->handle_length);

    pass &= encrypt_field(crypto, &sid, &sid_length,
                blob->service_id, blob->service_id_length);

    pass &= encrypt_field(crypto, &ticket, &ticket_length,
                blob->ticket, blob->ticket_length);

    if (!pass) {
        free(key   );
        free(handle);
        free(sid   );
        free(ticket);
        error->error   = VPL_ERR_FAIL;
        error->message = "The encryption failed.";
        return null;
    }

    length =
            3 * sizeof(int16_t)                         +
            5 * sizeof(int64_t)                         +
            packed_bytes_length (key_length           ) +
            packed_bytes_length (handle_length        ) +
            packed_bytes_length (sid_length           ) +
            packed_bytes_length (ticket_length        ) +
            packed_bytes_length (signature_size       ) +
            packed_string_length(blob->client_instance) +
            packed_string_length(blob->server_instance);

    if (blob->extra_length > 0) {
        length += packed_bytes_length (blob->extra_length);
    }

    signed_length = length - packed_bytes_length(signature_size);

    result = (char *) malloc(length);

    if (result == null) {
        free(key   );
        free(handle);
        free(sid   );
        free(ticket);

        error->error    = VPL_ERR_NOMEM;
        error->message  = "malloc in pxd_pack_blob";
        return null;
    }

    base    = result;
    end     = result + length;
    fail    = false;
    count   = blob_count;
    version = blob_version;
    fields  = blob_fields + (blob->extra_length > 0 ? 1 : 0);

    fail |= append_short (&base, end, count                         );
    fail |= append_short (&base, end, version                       );
    fail |= append_long  (&base, end, blob->client_user             );
    fail |= append_long  (&base, end, blob->client_device           );
    fail |= append_long  (&base, end, blob->server_user             );
    fail |= append_long  (&base, end, blob->server_device           );
    fail |= append_long  (&base, end, blob->create                  );
    fail |= append_short (&base, end, fields                        );
    fail |= append_bytes (&base, end, key,         key_length       );
    fail |= append_bytes (&base, end, handle,      handle_length    );
    fail |= append_bytes (&base, end, sid,         sid_length       );
    fail |= append_bytes (&base, end, ticket,      ticket_length    );
    fail |= append_string(&base, end, blob->client_instance         );
    fail |= append_string(&base, end, blob->server_instance         );

    if (blob->extra_length > 0) {
        fail |= append_bytes(&base, end, blob->extra, blob->extra_length);
    }

    if (!fail) {
        CSL_ResetSha (&context);
        CSL_InputSha (&context, crypto->sha1_key, sizeof(crypto->sha1_key));
        CSL_InputSha (&context, result, signed_length);
        CSL_ResultSha(&context, (u8 *) signature);
    }

    fail |= append_bytes (&base, end, signature,   signature_size   );

    free(key   );
    free(handle);
    free(sid   );
    free(ticket);

    if (fail || end - base != 0 || pxd_fail_pack) {
        error->error   = VPL_ERR_INVALID;
        error->message = "The blob buffer size was wrong.";
        free(result);
        result = null;
    } else {
        *length_out = length;
    }

    return result;
}


char *
pxd_pack_demon_blob
(
    int *           length_out,
    pxd_blob_t *    blob,
    pxd_crypto_t *  crypto,
    pxd_error_t *   error
)
{
    int     pass;
    int     fail;
    char *  result;
    char *  base;
    char *  end;
    int     length;
    char *  key;
    int     key_length;
    char *  handle;
    int     handle_length;
    char *  sid;
    int     sid_length;
    char *  ticket;
    int     ticket_length;
    char    signature[signature_size];
    int16_t count;
    int16_t fields;
    int16_t version;
    int     signed_length;

    CSL_ShaContext  context;

    *length_out = 0;

    pass  = encrypt_field(crypto, &key, &key_length,
                blob->key, blob->key_length);

    pass &= encrypt_field(crypto, &handle, &handle_length,
                blob->handle, blob->handle_length);

    pass &= encrypt_field(crypto, &sid, &sid_length,
                blob->service_id, blob->service_id_length);

    pass &= encrypt_field(crypto, &ticket, &ticket_length,
                blob->ticket, blob->ticket_length);

    if (!pass) {
        free(key   );
        free(handle);
        free(sid   );
        free(ticket);
        error->error   = VPL_ERR_FAIL;
        error->message = "The encryption failed.";
        return null;
    }

    length =
            3 * sizeof(int16_t)                         +
            3 * sizeof(int64_t)                         +
            packed_string_length(blob->client_instance) +
            packed_bytes_length (key_length           ) +
            packed_bytes_length (handle_length        ) +
            packed_bytes_length (sid_length           ) +
            packed_bytes_length (ticket_length        ) +
            packed_bytes_length (signature_size       );

    if (blob->extra_length > 0) {
        length += packed_bytes_length (blob->extra_length);
    }

    signed_length = length - packed_bytes_length(signature_size);

    result = (char *) malloc(length);

    if (result == null) {
        free(key   );
        free(handle);
        free(sid   );
        free(ticket);

        error->error    = VPL_ERR_NOMEM;
        error->message  = "malloc in pxd_pack_blob";
        return null;
    }

    base    = result;
    end     = result + length;
    fail    = false;
    count   = demon_count;
    version = demon_version;
    fields  = demon_fields + (blob->extra_length > 0 ? 1 : 0);

    fail |= append_short (&base, end, count                         );
    fail |= append_short (&base, end, version                       );
    fail |= append_long  (&base, end, blob->client_user             );
    fail |= append_long  (&base, end, blob->client_device           );
    fail |= append_long  (&base, end, blob->create                  );
    fail |= append_short (&base, end, fields                        );

    fail |= append_bytes (&base, end, key,         key_length       );
    fail |= append_bytes (&base, end, handle,      handle_length    );
    fail |= append_bytes (&base, end, sid,         sid_length       );
    fail |= append_bytes (&base, end, ticket,      ticket_length    );
    fail |= append_string(&base, end, blob->client_instance         );

    if (blob->extra_length > 0) {
        fail |= append_bytes(&base, end, blob->extra, blob->extra_length);
    }

    if (!fail) {
        CSL_ResetSha (&context);
        CSL_InputSha (&context, crypto->sha1_key, sizeof(crypto->sha1_key));
        CSL_InputSha (&context, result, signed_length);
        CSL_ResultSha(&context, (u8 *) signature);
    }

    fail |= append_bytes (&base, end, signature,   signature_size   );

    free(key   );
    free(handle);
    free(sid   );
    free(ticket);

    if (fail || end - base != 0 || pxd_fail_pack) {
        error->error    = VPL_ERR_FAIL;
        error->message  = "The ccd blob size was wrong.";
        free(result);
        result = null;
    } else {
        *length_out = length;
    }

    return result;
}

pxd_blob_t *
pxd_unpack_blob(char *buffer, int length, pxd_crypto_t *crypto, pxd_error_t *error)
{
    char *  base;
    char *  end;
    int     fail;
    int     signed_length;
    u8      expected[signature_size];
    u8   *  received;
    int     count;
    int     fields;
    int     signature_count;
    int     extras;

    CSL_ShaContext  context;
    pxd_blob_t *    blob;

    base    = buffer;
    end     = buffer + length;
    fail    = false;
    extras  = 0;

    signed_length = length - signature_size - sizeof(uint16_t);

    if (signed_length <= 0) {
        error->error   = VPL_ERR_INVALID;
        error->message = "The ccd blob is too small.";
        return null;
    }

    /*
     *  Compute the signature.
     */
    CSL_ResetSha (&context);
    CSL_InputSha (&context, crypto->sha1_key, sizeof(crypto->sha1_key));
    CSL_InputSha (&context, buffer, signed_length);
    CSL_ResultSha(&context, expected);

    /*
     *  Compute the address of the signature inside the packet.
     */
    received = (u8 *) end - signature_size;

    /*
     *  Check the expected signature and what we got.
     */
    if (memcmp(expected, received, signature_size) != 0) {
        pxd_blob_signatures++;
        error->error   = VPL_ERR_INVALID;
        error->message = "The ccd blob signature is invalid.";
        return null;
    }

    blob = (pxd_blob_t *) malloc(sizeof(*blob));

    if (blob == null) {
        error->error    = VPL_ERR_NOMEM;
        error->message  = "malloc failed in pxd_unpack_blob";
        return null;
    }

    memset(blob, 0, sizeof(*blob));

    unpack_u16_into      (count);
    unpack_u16_into      (blob->version);
    unpack_u64_into      (blob->client_user);
    unpack_u64_into      (blob->client_device);
    unpack_u64_into      (blob->server_user);
    unpack_u64_into      (blob->server_device);
    unpack_u64_into      (blob->create);
    unpack_u16_into      (fields);
    unpack_encrypted_into(blob->key,         blob->key_length,        "blob key");
    unpack_encrypted_into(blob->handle,      blob->handle_length,     "handle");
    unpack_encrypted_into(blob->service_id,  blob->service_id_length, "service id");
    unpack_encrypted_into(blob->ticket,      blob->ticket_length,     "ticket");
    unpack_string_into   (blob->client_instance);
    unpack_string_into   (blob->server_instance);

    for (int i = 0; i < fields - blob_fields; i++) {
        unpack_bytes_into(blob->extra, blob->extra_length);
        extras++;
    }

    unpack_u16_into      (signature_count);

    if (!fail) {
        fail =
                count         != blob_count
            ||  blob->version != blob_version
            ||  fields        != blob_fields + extras;
    }

    if
    (
        fail
    ||  signature_count != signature_size
    ||  end - base      != signature_size
    ) {
        error->error    = VPL_ERR_FAIL;
        error->message  = "The ccd blob parsing failed.";
        pxd_free_blob(&blob);
        return null;
    }

    return blob;
}


pxd_blob_t *
pxd_unpack_demon_blob(char *buffer, int length, pxd_crypto_t *crypto, pxd_error_t *error)
{
    char *  base;
    char *  end;
    int     fail;
    int     signed_length;
    u8      expected[signature_size];
    u8   *  received;
    int     count;
    int     fields;
    int     signature_count;
    int     extras;

    CSL_ShaContext  context;
    pxd_blob_t *    blob;

    base    = buffer;
    end     = buffer + length;
    fail    = false;
    extras  = 0;

    signed_length = length - signature_size - sizeof(uint16_t);

    if (signed_length <= 0) {
        error->error   = VPL_ERR_INVALID;
        error->message = "The pxd blob is too small.";
        return null;
    }

    /*
     *  Compute the signature.
     */
    CSL_ResetSha (&context);
    CSL_InputSha (&context, crypto->sha1_key, sizeof(crypto->sha1_key));
    CSL_InputSha (&context, buffer, signed_length);
    CSL_ResultSha(&context, expected);

    /*
     *  Compute the address of the signature inside the packet.
     */
    received = (u8 *) end - signature_size;

    /*
     *  Check the expected signature and what we got.
     */
    if (memcmp(expected, received, signature_size) != 0) {
        pxd_demon_blob++;
        error->error   = VPL_ERR_INVALID;
        error->message = "The pxd blob signature is invalid.";
        return null;
    }

    blob = (pxd_blob_t *) malloc(sizeof(*blob));

    if (blob == null) {
        error->error    = VPL_ERR_NOMEM;
        error->message  = "malloc failed in pxd_unpack_demon_blob";
        return null;
    }

    memset(blob, 0, sizeof(*blob));

    unpack_u16_into      (count);
    unpack_u16_into      (blob->version);
    unpack_u64_into      (blob->client_user);
    unpack_u64_into      (blob->client_device);
    unpack_u64_into      (blob->create);
    unpack_u16_into      (fields);
    unpack_encrypted_into(blob->key,         blob->key_length,         "key");
    unpack_encrypted_into(blob->handle,      blob->handle_length,      "handle");
    unpack_encrypted_into(blob->service_id,  blob->service_id_length,  "server id");
    unpack_encrypted_into(blob->ticket,      blob->ticket_length,      "ticket length");
    unpack_string_into   (blob->client_instance);

    for (int i = 0; i < fields - blob_fields; i++) {
        unpack_bytes_into(blob->extra, blob->extra_length);
        extras++;
    }

    unpack_u16_into      (signature_count);

    if (!fail) {
        fail =
                count         != demon_count
            ||  blob->version != demon_version
            ||  fields        != demon_fields + extras;
    }

    if
    (
        fail
    ||  signature_count != signature_size
    ||  end - base      != signature_size
    ) {
        error->error    = VPL_ERR_FAIL;
        error->message  = "The pxd blob parsing failed.";
        pxd_free_blob(&blob);
        return null;
    }

    return blob;
}

static int
pxd_queue
(
    pxd_io_t *      io,
    pxd_packet_t *  packet,
    pxd_error_t *   error,
    const char *    where,
    int             at_head
)
{
    int      slot;
    int      kill;
    int64_t  current_time;

    pxd_command_t  command;

    if (pxd_force_queue) {
        error->error    = VPL_ERR_INVALID;
        error->message  = "failed for testing";
        return false;
    }

    clear_error(error);
    memset(&command, 0, sizeof(command));   /* make gcc happy */
    packet->where = where;

    pxd_mutex_lock(&io->mutex);

    if (io->stop_now) {
        pxd_mutex_unlock(&io->mutex);
        error->error   = VPL_ERR_NOT_RUNNING;
        error->message = "The client is stopping.";
        return false;
    }

    slot             = -1;
    kill             = false;
    current_time     = VPLTime_GetTime();
    io->last_active  = current_time;

    /*
     *  Got through all the slots, looking for a free one, or one that
     *  corresponds to a timed-out operation.
     */
    for (int i = 0; i < array_size(io->outstanding) && slot < 0; i++) {
        if (!io->outstanding[i].valid) {
            slot = i;
        } else if (io->outstanding[i].start + io->time_limit < current_time) {
            slot    = i;
            kill    = true;
            command = io->outstanding[i];
        }
    }

    if (slot < 0) {
        pxd_mutex_unlock(&io->mutex);
        error->error   = VPL_ERR_BUSY;
        error->message = "The command slot table is full";
        return false;
    }

    /*
     *  Save the data in the slot.
     */
    io->outstanding[slot].valid     = true;
    io->outstanding[slot].async_id  = packet->async_id;
    io->outstanding[slot].opaque    = packet->opaque;
    io->outstanding[slot].type      = packet->type;
    io->outstanding[slot].start     = current_time;

    if (at_head) {
        packet->next   = io->queue_head;
        io->queue_head = packet;

        if (io->queue_tail == null) {
            io->queue_tail = packet;
        }
    } else if (io->queue_head == null) {
        io->queue_head = packet;
        io->queue_tail = packet;
    } else {
        io->queue_tail->next = packet;
        io->queue_tail       = packet;
    }

    pxd_mutex_unlock(&io->mutex);

    if (kill) {
        io->kill(io, command, pxd_timed_out);
    }

    pxd_ping(io->event, where);
    return true;
}

int
pxd_prequeue_packet(pxd_io_t *io, pxd_packet_t *packet, pxd_error_t *error, const char *where)
{
    return pxd_queue(io, packet, error, where, true);
}

int
pxd_queue_packet(pxd_io_t *io, pxd_packet_t *packet, pxd_error_t *error, const char *where)
{
    return pxd_queue(io, packet, error, where, false);
}

/*
 *  This routine removes the packet at the head of the output
 *  queue, frees the memory associated with it, and then returns
 *  the next available packet, or null, if the queue is empty.
 *
 *  It is called when the thread finishes writing a packet or when
 *  some sort of failure requires flushing the queue.
 */
static pxd_packet_t *
advance_queue(pxd_io_t *io, int success)
{
    pxd_packet_t *  packet;
    pxd_packet_t *  next;

    packet          = io->queue_head;
    io->queue_head  = packet->next;
    next            = packet->next;

    /*
     *  Clear the tail pointer if the queue is empty now.
     */
    if (io->queue_head == null) {
        io->queue_tail = null;
    }

    if (packet != null && success) {
        pxd_packets_out++;  // for testing
    }

    pxd_free_packet(&packet);

    return next;
}

/*
 *  Try to do some asynchronous output, if there's anything
 *  on the queue.
 */
void
pxd_write_output(pxd_io_t *io)
{
    int  count;
    int  done;

    pxd_packet_t *  packet;

    pxd_mutex_lock(&io->mutex);

    packet = io->queue_head;

    if (packet == null || !io->ready_to_send) {
        pxd_mutex_unlock(&io->mutex);
        return;
    }

    io->last_active = VPLTime_GetTime();

    /*
     *  If this packet hasn't had the sequence number inserted and
     *  this signature generated, do that now.
     */
    pxd_prep_packet(io, packet, &io->out_sequence);
    pxd_mutex_unlock(&io->mutex);

    /*
     *  Okay, write until the socket is full or we're out of packets.
     */
    do {
        count = packet->length;

        if (pxd_write_limit != 0 && count > pxd_write_limit) { // for testing
            count = pxd_write_limit;
        }

        count = VPLSocket_Send(io->socket, packet->buffer, count);
        done  = count <= 0;

        /*
         *  We should get here only when the socket fails between the
         *  poll and the VPLSocket_Send call.  Pause a bit to prevent
         *  any possible compute loop if something strange happens.
         */
        if (done) {
            VPLThread_Sleep(VPLTime_FromMillisec(200));
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
            packet->buffer = null;

            if (packet->type != Send_timed_ping) {
                log_info("connection " FMTs64 "@%s sent      %s (%d), op id " FMTu64 " (%s)",
                    io->connection,
                    io->host,
                    pxd_packet_type(packet->type),
                    packet->type,
                    packet->async_id,
                    packet->where);
            }

            pxd_done(packet, true);

            /*
             *  Get the next packet from the queue, if there is one.
             */
            pxd_mutex_lock(&io->mutex);
            ASSERT(packet == io->queue_head);

            packet = advance_queue(io, VPL_TRUE);
            done   = packet == null;

            if (done) {
                pxd_mutex_unlock(&io->mutex);
                break;
            }

            /*
             *  If there's more to write, prep the buffer and add
             *  the sequence number, if we can.
             */
            packet->prepared = false;
            pxd_prep_packet(io, packet, &io->out_sequence);
            pxd_mutex_unlock(&io->mutex);
        }
    } while (!done);

    return;
}

/*
 *  Read from the TCP socket.  Convert EOF to an error to
 *  close the socket.  Ignore VPL_ERR_AGAIN and VPL_ERR_INTR
 *  error returns.
 */
static int
do_read(pxd_io_t *io, void *buffer, int bytes)
{
    int  count;

    count = VPLSocket_Recv(io->socket, buffer, bytes);

    if (count < 0 && count != VPL_ERR_AGAIN && count != VPL_ERR_INTR) {
        log_info("connection " FMTs64 "@%s failed    count %d.",
            io->connection, io->host, (int) count);
    } else if (count == 0) {
        log_info("connection " FMTs64 "@%s closed",
            io->connection, io->host);
        count = -1; // force an error on when the server closes the socket
    } else if (count < 0) {
        count = 0;
    }

    return count;
}

/*
 *  Compute the timeout for a poll on the TCP and event
 *  sockets.  We need to obey the partial packet timeout
 *  limit and the print interval.
 */
static uint64_t
compute_timeout
(
    pxd_io_t *  io,
    uint64_t    last_ping,
    uint64_t    last_print,
    int         doing_timeout,
    uint64_t    start_time
)
{
    uint64_t  current_time;
    uint64_t  wait_time;
    uint64_t  interval;
    uint64_t  time_waited;

    current_time = VPLTime_GetTime();
    current_time = pxd_max(current_time, start_time);
    wait_time    = VPLTime_FromSec(200000);

#ifdef pxd_print
    if (pxd_print_interval > 0 && current_time >= last_print) {
        interval    = VPLTime_FromSec(pxd_print_interval);
        time_waited = current_time - last_print;

        if (time_waited < interval) {
            wait_time = pxd_min(wait_time, interval - time_waited);
        } else {
            wait_time = 0;
        }
    }
#endif

    if (doing_timeout && pxd_partial_timeout > 0) {
        interval    = VPLTime_FromSec(pxd_partial_timeout);
        time_waited = current_time - start_time;

        if (time_waited < interval) {
            wait_time = pxd_min(wait_time, interval - time_waited);
        } else {
            wait_time = 0;
        }
    }

    /*
     *  If there's an idle limit, reduce the wait time to
     *  a fraction of that to try to obey it.
     */
    if (io->idle_limit > 0) {
        wait_time = pxd_min(wait_time, io->idle_limit / 4);
    }

    /*
     *  Wait times less than 1 ms don't seem to work well on Linux.
     *  They seem to lead to a zero wait time.  100 ms should be okay.
     */
    wait_time = pxd_max(wait_time, VPLTime_FromMillisec(100));
    return wait_time;
}

static void
pxd_check_timeout(pxd_io_t *io)
{
    int      i;
    int      timeout_count;
    int64_t  current_time;

    pxd_command_t   timeouts[outstanding_limit];

    timeout_count = 0;
    current_time  = VPLTime_GetTime();

    pxd_mutex_lock(&io->mutex);

    /*
     *  Loop through the array of all operations.
     */
    for (i = 0; i < array_size(io->outstanding); i++) {
        if
        (
            io->outstanding[i].valid
        &&  io->outstanding[i].start + io->time_limit < current_time
        ) {
            timeouts[timeout_count++]  = io->outstanding[i];
            io->outstanding[i].valid   = false;
        }
    }

    pxd_mutex_unlock(&io->mutex);

    for (i = 0; i < timeout_count; i++) {
        io->kill(io, timeouts[i], pxd_timed_out);
    }
}

/*
 *  Loop doing I/O on the TCP socket.  This routine is passed a buffer
 *  and a count, and returns only when the buffer is full, or some sort
 *  of error or shutdown occurs.
 */
static int
io_loop(pxd_io_t *io, void *buffer_in, int expected, int reading_initial)
{
    int        count;
    int        total;
    VPLTime_t  timeout;
    VPLTime_t  last_ping;
    VPLTime_t  last_print;
    VPLTime_t  start_time;
    VPLTime_t  current_time;
#ifdef pxd_print
    VPLTime_t  print_interval;
#endif
    VPLTime_t  partial_timeout;
    int        failed;
    int        result;
    int        do_timeout;
    char *     buffer;

    VPLSocket_poll_t  poll[2];

    buffer          = (char *) buffer_in;
    total           = 0;
    start_time      = VPLTime_GetTime();
    last_ping       = start_time;
    last_print      = start_time;
    partial_timeout = VPLTime_FromSec(pxd_partial_timeout);

    do {
        do_timeout      = !(reading_initial && total == 0) && partial_timeout > 0;
        timeout         = compute_timeout(io, last_ping, last_print, do_timeout, start_time);

        poll[0].socket  = io->socket;
        poll[0].events  = io->readable ? VPLSOCKET_POLL_RDNORM : 0;
        poll[0].revents = 0;

        poll[1].socket  = pxd_socket(io->event);
        poll[1].events  = VPLSOCKET_POLL_RDNORM;
        poll[1].revents = 0;

        /*
         *  Check whether we have a packet to send, and whether
         *  it's valid to try to send one.  If we've received a
         *  packet, the server is ready to receive them. Otherwise,
         *  maybe not.
         */
        if (io->queue_head != null && io->ready_to_send) {
            poll[0].events |= VPLSOCKET_POLL_OUT;
        }

        /*
         *  Do the actual poll, then check the event structure and
         *  the output situation.
         */
        result = VPLSocket_Poll(poll, array_size(poll), timeout);

        pxd_check_event(io->event, result, &poll[1]);
        pxd_check_timeout(io);

        if (poll[0].revents & VPLSOCKET_POLL_OUT) {
            pxd_write_output(io);
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
         *  Check for I/O errors on the TCP socket.
         */
        if (poll[0].revents & (VPLSOCKET_POLL_ERR | VPLSOCKET_POLL_HUP)) {
            log_info("connection " FMTs64 "@%s failed    poll flags 0x%x",
                      io->connection,
                      io->host,
                (int) poll[0].revents);
            total = -1;
            break;
        }

        /*
         *  Now check various timers.
         */
        current_time = VPLTime_GetTime();

#ifdef pxd_print
        if (pxd_print_interval > 0) {
            print_interval = VPLTime_FromSec(pxd_print_interval);
        } else {
            print_interval = 0;
        }

        if (print_interval > 0 && current_time - last_print > print_interval) {
            last_print = current_time;
            pxd_print_count++;
            log_info_1("I am waiting for data.");
        }
#endif

        if
        (
            current_time - start_time > partial_timeout
        &&  do_timeout
        ) {
            log_warn("connection " FMTs64 "@%s reached the partial packet timeout.",
                io->connection, io->host);
            pxd_partial_timeouts++;
            total = -1;
            break;
        }

        /*
         *  Read when the poll says there's input.
         */
        if (poll[0].revents & VPLSOCKET_POLL_RDNORM) {
            count = do_read(io, buffer, expected - total);

            if (count < 0) {
                total = -1;
                break;
            }

            buffer += count;
            total  += count;
        }

        /*
         *  If an idle limit has been set, obey it!
         */
        pxd_mutex_lock(&io->mutex);

        if
        (
            io->idle_limit > 0
        &&  current_time > io->last_active + io->idle_limit
        &&  io->queue_head == null
        ) {
            log_info("closing idle connection " FMTs64 "@%s.",
                io->connection, io->host);
            io->stop_now = true;
        }

        pxd_mutex_unlock(&io->mutex);
    } while (total < expected && !io->stop_now);

    return total;
}

/*
 *  Read a TCP packet from the network.  This routine returns
 *  only when a packet has been read or an error or shutdown
 *  has occurred.
 */
pxd_packet_t *
pxd_read_packet(pxd_io_t *io)
{
    pxd_packet_t *  packet;

    char *  body;
    char *  base;
    u16     packet_length;
    u16     check_length;
    int     net_check;
    int     net_length;
    int     remaining;
    int     result;

    /*
     *  First, read the packet length and check length,
     *  which are the first four bytes on the network.
     *  The check length comes first.
     */
    result = io_loop(io, &packet_length, sizeof(packet_length), VPL_TRUE);

    if (io->stop_now) {
        return null;
    }

    if (result != sizeof(packet_length)) {
        return null;
    }

    /*
     *  Get the check length.  It's the first two bytes of
     *  the packet.
     */
    check_length = ~VPLConv_ntoh_u16(packet_length);

    /*
     *  Read the length field.  It's next.
     */
    result = io_loop(io, &packet_length, sizeof(packet_length), VPL_FALSE);

    if (io->stop_now || result != sizeof(packet_length)) {
        return null;
    }

    /*
     *  Convert the packet length to host format, and
     *  start checking.
     */
    packet_length = VPLConv_ntoh_u16(packet_length);

    if (packet_length < header_size || packet_length > pxd_max_packet_size) {
        pxd_packet_size_errors++;
        log_error("connection " FMTs64 "@%s failed on the packet size check (%d).",
                  io->connection,
                  io->host,
            (int) packet_length);
        return null;
    }

    if (check_length != packet_length) {
        pxd_size_mismatches++;
        log_error("connection " FMTs64 "@%s:  The lengths didn't match (%d vs %d).",
            io->connection, io->host,
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
     *  Okay, put the packet together.  We need to put the length and
     *  the check length back into it, in proper network form, so that
     *  we can compute the correct signature and check it against what
     *  we received.
     */
    remaining   = check_length - 2 * sizeof(uint16_t);
    net_check   = VPLConv_ntoh_u16(~check_length);
    net_length  = VPLConv_ntoh_u16( check_length);

    memcpy(&body[0],                    &net_check,  sizeof(check_length));
    memcpy(&body[sizeof(check_length)], &net_length, sizeof(check_length));

    base = &body[2 * sizeof(check_length)];

    result = io_loop(io, base, remaining, VPL_FALSE);

    if (result != remaining) {
        log_error("connection " FMTs64 "@%s failed trying to read the body:  "
            "%d bytes received, expected %d.",
            io->connection,
            io->host,
            result,
            remaining);
        free(body);
        return null;
    }

    packet = pxd_alloc_packet(body, packet_length, null);

    if (packet == null){
        log_error_1("I failed to malloc the packet structure.");
        free(body);
        return null;
    }

    io->last_active = VPLTime_GetTime();
    return packet;
}

void
pxd_kill_all(pxd_io_t *io, int result)
{
    int  i;
    int  timeouts;

    timeouts = 0;

    pxd_mutex_lock(&io->mutex);

    for (i = 0; i < array_size(io->outstanding); i++) {
        if (io->outstanding[i].valid) {
            io->timeouts   [timeouts++] = io->outstanding[i];
            io->outstanding[i].valid = false;
        }
    }

    io->timeouts_count = timeouts;
    pxd_mutex_unlock(&io->mutex);

    for (i = 0; i < io->timeouts_count; i++) {
        io->kill(io, io->timeouts[i], result);
    }
}

extern void
pxd_configure_packet(pxd_client_t *client, pxd_unpacked_t *unpacked)
{
    pxd_update_param(unpacked->max_packet_size, &pxd_max_packet_size, "packet size"    );
    pxd_update_param(unpacked->max_encrypt    , &pxd_max_encrypt    , "proxy retries"  );
    pxd_update_param(unpacked->partial_timeout, &pxd_partial_timeout, "partial timeout");
}

extern void
pxd_update_param(int value,  int *where, const char *name)
{
    if (value > 0) {
        *where = value;
        log_info("updated the \"%s\" parameter to %d", name, value);
    }
}
