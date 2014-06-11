//
//  Copyright 2011-2013 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#include <malloc.h>
#include <ans_device.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef IOS
#include <malloc.h>
#endif
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <ans_device.h>

#define null   ((void *) 0)
#define false  0
#define true   1

#define ASSERT(x)
#define LOG_FUNC_ENTRY(x)
#define LOG_INFO(x, args...)    printf(x, ## args); printf("\n");
#define LOG_ERROR(x, args...)   printf(x, ## args); printf("\n");
#define LOG_WARN(x, args...)    printf(x, ## args); printf("\n");
#define LOG_ALWAYS(x, args...)  printf(x, ## args); printf("\n");

#define log(x, args...)         ts(); printf("test: "); printf(x, ## args)
#define min(a, b)               ((a) < (b) ? (a) : (b))
#define null                    ((void *) 0)

#define SERVER_TCP_PORT         443

#define ans_test

static void ts(void);

#include <ans_device.cpp>

static int  login_active   = false;
static int  test_completed = false;

/*
 *  Implement the ANS device library callbacks.
 */

static int
t_connection_active(ans_client_t *client, VPLNet_addr_t address)
{
    return 1;
}

static void
t_connection_down(ans_client_t *client)
{
    if (!test_completed) {
        login_active = false;
        log("*** The connection failed.\n");
        exit(1);
    }
}

static int
t_receive_notification(ans_client_t *client, ans_unpacked_t *unpacked)
{
}

static void
t_receive_sleep_info(ans_client_t *client, ans_unpacked_t *unpacked)
{
}

static void
t_receive_device_state(ans_client_t *client, ans_unpacked_t *unpacked)
{
}

static void
t_connection_closed(ans_client_t *client)
{
    login_active = false;
}

static void
t_set_ping_packet(ans_client_t *client, char *ping_packet, int ping_length)
{
}

static void
t_reject_credentials(ans_client_t *client)
{
    log("*** The credentials are invalid\n");
    exit(1);
}

static void
t_login_completed(ans_client_t *client)
{
    login_active = true;
}

static void
t_reject_subscriptions(ans_client_t *client)
{
}

static void
setup_callbacks(ans_callbacks_t *callbacks)
{
    memset(callbacks, 0, sizeof(*callbacks));

    callbacks->connectionActive    = t_connection_active;
    callbacks->receiveNotification = t_receive_notification;
    callbacks->receiveSleepInfo    = t_receive_sleep_info;
    callbacks->receiveDeviceState  = t_receive_device_state;
    callbacks->connectionDown      = t_connection_down;
    callbacks->connectionClosed    = t_connection_closed;
    callbacks->setPingPacket       = t_set_ping_packet;
    callbacks->rejectCredentials   = t_reject_credentials;
    callbacks->loginCompleted      = t_login_completed;
    callbacks->rejectSubscriptions = t_reject_subscriptions;
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
        log("*** unmarshal a null string\n");
        exit(1);
    }

    binary = (char *) malloc(output_length);

    if (binary == NULL) {
        log("*** unmarshal malloc failed.\n");
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

int
main(int argc, char **argv)
{
    ans_open_t       input;
    const char *     ans_host    = getenv("ans_host");
    const char *     ans_user    = getenv("ans_user");
    const char *     ans_device  = getenv("ans_device");
    const char *     ans_key     = getenv("ans_key");
    const char *     ans_blob    = getenv("ans_blob");
    int              die;
    uint64_t         user_id;
    uint64_t         device_id;
    char *           blob;
    int              blob_length;
    char *           key;
    int              key_length;
    int              loops;
    int              background_interval;
    int              my_socket;
    int              round;
    int              pass;
    ans_client_t *   client;
    ans_callbacks_t  callbacks;

    setlinebuf(stdout);
    die = false;

    ans_host = getenv("ans_host");

    if (ans_host == null) {
        log("*** ans_host must be set\n");
        die = true;
    }

    if (ans_user == null) {
        log("*** ans_user must be set\n");
        die = true;
    }

    if (ans_device == null) {
        log("*** ans_device must be set\n");
        die = true;
    }

    if (ans_key == null) {
        log("*** ans_key must be set\n");
        die = true;
    }

    if (ans_blob == null) {
        log("*** ans_blob must be set\n");
        die = true;
    }

    if (die) {
        exit(1);
    }

    blob = unmarshal(ans_blob, &blob_length);
    key  = unmarshal(ans_key,  &key_length);

    setup_callbacks(&callbacks);

    input.clusterName   = ans_host;
    input.callbacks     = &callbacks;
    input.blob          = blob;
    input.blobLength    = blob_length;
    input.key           = key;
    input.keyLength     = key_length;
    input.deviceType    = "default";
    input.application   = "ans_circle";
    input.verbose       = true;
    input.server_tcp_port = SERVER_TCP_PORT;

    client = ans_open(&input);

    if (client == null) {
        log("ans_open failed\n");
        exit(1);
    }

    loops = 0;

    while (!login_active && loops++ < 5) {
        sleep(1);
    }

    my_socket = client->keep.live_socket_id;

    if (!login_active || my_socket < 0) {
        log("*** The login failed.\n");
        exit(1);
    }

    background_interval = 90;
    pass = ans_setForeground(client, false, background_interval * 1000 * 1000);

    if (!pass) {
        log("*** I failed to set background mode\n");
        exit(1);
    }

    for (round = 0; round < 4; round++) {
        sleep(background_interval);

        if (!login_active || my_socket != client->keep.live_socket_id) {
            log("*** The connection failed during sleep %d.", round + 1);
        }

        ans_background(client);
    }

    pass = ans_setForeground(client, true, 0);

    if (!pass) {
        log("*** I failed to set background mode\n");
        exit(1);
    }

    sleep(background_interval);

    if (!login_active || my_socket != client->keep.live_socket_id) {
        log("*** The connection failed during sleep %d.", round + 1);
    }

    test_completed = true;
    ans_close(client, true);
    exit(0);
}

static void
ts(void)
{
    struct tm  tm;
    time_t     timeval;

    timeval = time(NULL);
    gmtime_r(&timeval, &tm);

    if (tm.tm_hour == 0) {
        tm.tm_hour = 24;
    }

    tm.tm_year += 1900;

    printf("%4d/%02d/%02d %02d:%02d:%02d UTC  ",
        tm.tm_year, tm.tm_mon + 1, tm.tm_mday,
        tm.tm_hour, tm.tm_min, tm.tm_sec);
}
