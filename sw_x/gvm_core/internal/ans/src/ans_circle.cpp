//
//  Copyright 2011-2013 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

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

#undef min
#undef ASSERT
#undef LOG_FUNC_ENTRY
#undef LOG_INFO
#undef LOG_ERROR
#undef LOG_WARN
#undef LOG_ALWAYS

#define ASSERT(x)
#define LOG_FUNC_ENTRY(x)
#define LOG_INFO(x, args...)    printf(x, ## args); printf("\n");
#define LOG_ERROR(x, args...)   printf(x, ## args); printf("\n");
#define LOG_WARN(x, args...)    printf(x, ## args); printf("\n");
#define LOG_ALWAYS(x, args...)  printf(x, ## args); printf("\n");

#define log(x, args...)         ts(); printf("test: "); printf(x, ## args)
#define min(a, b)               ((a) < (b) ? (a) : (b))
#define null                    ((void *) 0)

#define ans_check_signature
#define ans_test
#define ans_send_ping
#define req_device_count  2

#define SERVER_TCP_PORT         443

#define streq(a, b) (strcmp((a), (b)) == 0)

#include <ans_device.cpp>

static ans_client_t *     main_client;
static ans_client_t *     target_client;
static volatile int       received_count     = 0;
static volatile int       device_updates     = 0;
static volatile int       target_updates     = 0;
static volatile int       target_state_ops   = 0;
static volatile int       target_state       = 0;
static volatile int       target_online      = false;
static volatile uint64_t  target_sleep_time  = 0;
static volatile uint64_t  logins_completed   = 0;
static volatile int       main_down_events   = 0;
static volatile int       main_down_okay     = false;
static volatile int       exiting            = false;
static volatile int       expecting_reject   = false;
static volatile int       sleep_setups;
static volatile int       wakeups_to_receive = 0;
static volatile int       unicasts           = 0;
static volatile int       multicasts         = 0;
static volatile int       udp_reader_started = false;
static const char *       unicast_text       = "self unicast";
static const char *       multicast_text     = "self multicast";
static const char *       udp_host;
static const char *       target_host;
static volatile int       udp_fd;
static volatile int       responses          = 0;
static volatile uint64_t  query_id           = 543;
static uint64_t           req_device_list[req_device_count];
static volatile uint64_t  user_id;
static volatile uint64_t  main_device_id;
static volatile uint64_t  target_device_id;
static char               mac_address[6]     = { 1, 2, 3, 4, 5, 6 };
static char               invalid_mac[256];
static char *             sleep_packet       = NULL;
static volatile int       sleep_length       = 1024 + 24 + 14;  //TODO

static volatile uint64_t  rejected_subscriptions = 0;

static int                udp_server_length;
static struct sockaddr_in udp_server_address;


static char *    blob;
static int       blob_length;
static char *    key;
static int       key_length;
static int       do_ping;
static int       do_test_misc;
static int       do_sleep_test;

static void
ts(void)
{
    char  buffer[500];

    printf("%s  ", utc_ts(buffer, sizeof(buffer)));
}

static const char *
get_state_text(int state)
{
    const char *  state_name;

    switch (state) {
    case DEVICE_ONLINE:
        state_name = "online";
        break;

    case DEVICE_SLEEPING:
        state_name = "sleeping";
        break;

    case DEVICE_OFFLINE:
        state_name = "offline";
        break;

    default:
        log("device state %d is not known\n", state);
        state_name = "??unknown??";
    }

    return state_name;
}

/*
 *  Implement the ANS device library callbacks.
 */

static int
t_connection_active(ans_client_t *client, VPLNet_addr_t address)
{
    log("connection active for main client %p\n", client);
    return 1;
}

static int
t_connection_active_target(ans_client_t *client, VPLNet_addr_t address)
{
    log("connection active for target client %p\n", client);
    return 1;
}

static void
t_connection_down_main(ans_client_t *client)
{
    log("connection down on the main client %p\n", client);
    main_down_events++;

    if (!main_down_okay) {
        log("*** The main connection (client %p) failed!\n", client);
        exit(1);
    }
}

static void
t_connection_down_target(ans_client_t *client)
{
    target_online = false;
    log("connection down\n");
}

static int
t_receive_notification(ans_client_t *client, ans_unpacked_t *unpacked)
{
    char *  message;

    /*
     *  Validate the user id in the message.
     */
    if (user_id != unpacked->userId) {
        log("*** The user ids don't match:  %lld vs %lld\n",
            user_id, unpacked->userId);
        exit(1);
    }

    /*
     *  Make a null-terminated copy of the message and print it.  Also,
     *  increment the appropriate counter.
     */
    if (unpacked->type == SEND_USER_NOTIFICATION ||
            unpacked->type == SEND_USER_MULTICAST) {
        message = (char *) malloc(unpacked->notificationLength + 1);
        memcpy(message, unpacked->notification, unpacked->notificationLength);
        message[unpacked->notificationLength] = 0;

        if (streq(message, unicast_text)) {
            log("got self unicast:  %s\n", (const char *) message);
            unicasts++;
        } else if (streq(message, multicast_text)) {
            log("got self multicast:  %s\n", (const char *) message);
            multicasts++;
        } else if (client == main_client) { 
            log("got message:  %s\n", (const char *) message);
            received_count++;
        }

        free(message);
    }

    return 1;
}

static void
t_receive_sleep_info_main(ans_client_t *client, ans_unpacked_t *unpacked)
{
    if (unpacked->type == SEND_SLEEP_SETUP) {
        log("received the sleep setup packet\n");
        sleep_setups++;

        if (unpacked->ioacType != ioac_proto) {
            log("*** The ioac type didn't match:  %d vs %d\n",
                (int) ioac_proto, (int) unpacked->ioacType);
            exit(1);
        }

        if (unpacked->macAddressLength != sizeof(mac_address)) {
            log("*** The mac length didn't match:  %d vs %d\n",
                (int) sizeof(mac_address), (int) unpacked->macAddressLength);
            exit(1);
        }

        if (memcmp(unpacked->macAddress, mac_address, sizeof(mac_address)) != 0) {
            log("The mac addresses didn't match.\n");
        }
    }
}

/*
 *  Receive the sleep information for the target client.  The sleep packet
 *  is saved for further testing.
 */
static void
t_receive_sleep_info_target(ans_client_t *client, ans_unpacked_t *unpacked)
{
    char *  work;

    log("got the sleep info -- type %d\n", unpacked->type);

    if (sleep_packet != NULL) {
        return;
    }

    log("received a version 2 sleep packet of length %d\n", (int) unpacked->sleepPacketLength);

    if (unpacked->type == SEND_SLEEP_SETUP) {
        sleep_length = unpacked->sleepPacketLength;
        work         = (char *) malloc(sleep_length);

        memcpy(work, unpacked->sleepPacket, sleep_length);

        sleep_packet = work;
    } else {
        log("*** That sleep type is invalid!\n");
        exit(1);
    }
}

static void
handle_device_update(ans_client_t *client, ans_unpacked_t *unpacked)
{
    /*
     *  We got a device update packet, or at least that what it should
     *  be.
     */
    if (unpacked->type != SEND_DEVICE_UPDATE) {
        log("received packet type %d; it should be %d\n",
            (int) unpacked->type, (int) SEND_STATE_LIST);
        exit(1);
    }

    /*
     *  If this was a device update (just one device), print the
     *  result.
     */
    log("got device update %d (%s) for device " FMTu64 "\n",
        unpacked->newDeviceState, get_state_text(unpacked->newDeviceState),
            unpacked->deviceId);

    /*
     *  Finally, if this is a device update, and it's about the target device,
     *  save the state.
     */
    if (unpacked->deviceId == target_device_id) {
        target_state      = unpacked->newDeviceState;
        target_sleep_time = unpacked->newDeviceTime;
        target_updates++;

        log("setting target_state to %d\n", (int) target_state);
        log("setting target_sleep_time to " FMTu64 "\n",
            target_sleep_time);
    }

    /*
     *  Check that the async id was returned properly, if the async id
     *  is set.
     */
    if (unpacked->asyncId != query_id && unpacked->asyncId != 0) {
        log("*** got query id %lld, expected %lld\n",
           (long long) unpacked->asyncId, (long long) query_id);
        exit(1);
    }
}

static void
t_receive_device_state(ans_client_t *client, ans_unpacked_t *unpacked)
{
    int  i;

    device_updates++;

    /*
     *  If the test is over, ignore the state.
     */
    if (exiting) {
        return;
    }

    /*
     *  If we got a device update, without a state list, handle
     *  that elsewhere.
     */
    if (unpacked->deviceList == NULL) {
        handle_device_update(client, unpacked);
        return;
    }

    if (unpacked->type != SEND_STATE_LIST) {
        log("*** received packet type %d; it should be %d\n",
            (int) unpacked->type, (int) SEND_STATE_LIST);
        exit(1);
    }

    log("got a device state array:  {");

    for (i = 0; i < unpacked->deviceCount; i++) {
       printf("%s %d", (i == 0 ? "" : ","), (int) unpacked->deviceStates[i]);
    }

    printf(" }, id " FMTu64 "\n", unpacked->asyncId);

    /*
     *  We might get a query for the req_device_list array or for the
     *  target device.  Verify each case separately.
     */
    if (unpacked->deviceCount == array_size(req_device_list)) {
        for (i = 0; i < unpacked->deviceCount; i++) {
            if (unpacked->deviceList[i] != req_device_list[i]) {
                log("*** got device id " FMTu64 "\n", unpacked->deviceList[i]);
                exit(1);
            }

            log("got device state %d (%s) for " FMTu64 " in the array\n",
                (int) unpacked->deviceStates[i],
                get_state_text(unpacked->deviceStates[i]),
                unpacked->deviceList[i]);
        }

        if (unpacked->deviceStates[0] != DEVICE_ONLINE) {
            log("*** got device state %d (%s) for " FMTu64 "\n",
                (int) unpacked->deviceStates[0],
                get_state_text(unpacked->deviceStates[0]),
                unpacked->deviceList[0]);
                exit(1);
        }
    } else {
        if (unpacked->deviceCount != 1 || unpacked->deviceList[0] != target_device_id) {
            log("*** got an invalid device list\n");
            exit(1);
        }

        target_state      = unpacked->deviceStates[0];
        target_sleep_time = unpacked->deviceTimes[0];
        target_updates++;

        log("setting target_state to %d (list)\n", (int) target_state);
        log("setting target_sleep_time to " FMTu64 " (list)\n",
            target_sleep_time);
    }

    /*
     *  Check that the async id was returned properly, if the async id
     *  is set.
     */
    if (unpacked->asyncId != query_id && unpacked->asyncId != 0) {
        log("*** got query id %lld, expected %lld\n",
           (long long) unpacked->asyncId, (long long) query_id);
        exit(1);
    }
}

static void
t_receive_device_state_target(ans_client_t *client, ans_unpacked_t *unpacked)
{
    log("receive target device state\n");
    target_state_ops++;
    t_receive_device_state(client, unpacked);
}

static void
t_connection_closed_main(ans_client_t *client)
{
}

static void
t_connection_closed_target(ans_client_t *client)
{
    target_online = false;
}

static void
t_set_ping_packet(ans_client_t *client, char *ping_packet, int ping_length)
{
    int  i;

    log("set the ping packet:  0x");

    for (i = 0; i < ping_length; i++) {
        printf("%02x", ping_packet[i]);
    }

    printf("\n");
}

static void
t_reject_credentials(ans_client_t *client)
{
    log("The ANS credentials were rejected.\n");

    if (expecting_reject) {
        expecting_reject = false;
    } else {
        log("*** The rejection was not expected.\n");
        exit(1);
    }
}

static void
t_login_completed(ans_client_t *client)
{
    if (client == target_client) {
        log("The target login completed.\n");
        target_online = true;
    } else {
        log("The login attempt has completed.\n");
    }


    if (client == main_client && client->device_id != 0 && client->device_id != main_device_id) {
        log("*** The client has device id " FMTu64 ", but it should be " FMTu64 "\n",
            client->device_id, main_device_id);
        exit(1);
    }

    logins_completed++;
}

static void
t_reject_subscriptions(ans_client_t *client)
{
    log("The subscriptions were rejected.\n");
    rejected_subscriptions++;
}

static void
t_receive_response(ans_client_t *client, ans_unpacked_t *unpacked)
{
    responses++;
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

    memset(&local_name, 0, sizeof(local_name));
    local_length = sizeof(local_name);
    ip_address   = NULL;

    result = getsockname(fd, &local_name, &local_length);

    if (result < 0) {
        log("getsockname failed\n");
        ts();
        fflush(stdout);
        perror("getsockname");
    } else if (local_name.sa_family == PF_INET) {
        ip_name     = (struct sockaddr_in *) &local_name;
        ip_address  = ip_print((char *) &ip_name->sin_addr.s_addr, 4);
        ip_port     = ip_name->sin_port;

        log("using IP address %s:%d\n",
            ip_address, (int) ip_port);
    } else if (local_name.sa_family == PF_INET6) {
        ipv6_name   = (struct sockaddr_in6 *) &local_name;
        ip_address  = ip_print((char *) &ipv6_name->sin6_addr.s6_addr, 16);
        ip_port     = ipv6_name->sin6_port;

        log("using IPv6 address %s:%d\n",
            ip_address, (int) ip_port);
    }

    free(ip_address);
}

static void
request_device_state(ans_client_t *client, uint64_t query_id, uint64_t *device_list, int count, const char *where)
{
    int  result;
    int  loops;

    loops = 0;

    do {
        result = ans_requestDeviceState(client, query_id, device_list, count);

        if (!result) {
            log("Sleeping while sending the device query (%s)\n", where);
            sleep(1);
        }
    } while (!result && loops++ < 20);

    if (!result) {
        log("*** The device query could not be sent (%s)\n", where);
        exit(1);
    }

    return;
}

/*
 *  Test some functions to see that they at least don't dump core.
 */
static void
test_misc()
{
    log("using main device id " FMTu64 "\n", main_device_id);

    log("====== Starting the miscellaneous test.\n");

    if (!ans_setSubscriptions(main_client, 0, req_device_list, req_device_count)) {
        log("*** set subscriptions for the main client failed\n");
        exit(1);
    }

    request_device_state(main_client, query_id, req_device_list, req_device_count, "misc");

    if (!ans_requestWakeup(main_client, req_device_list[0])) {
        log("*** request wakeup failed\n");
        exit(1);
    }

    log("====== Finished the miscellaneous test.\n");
}

/*
 *  This function receives and counts wakeup packets.  TODO  It should validate
 *  the contents.
 */
static VPLTHREAD_FN_DECL
udp_reader(void *data)
{
    int   result;
    char  buffer[80];

    udp_reader_started = true;

    do {
        result = read(udp_fd, buffer, sizeof(buffer));

        if (result > 0) {
            wakeups_to_receive--;
            log("got a wakeup packet\n");
        } else {
            ts();
            fflush(stdout);
            perror("test: udp read");
        }
    } while (result > 0 && wakeups_to_receive > 0);

    print_socket_name(udp_fd);
    return VPLTHREAD_RETURN_VALUE;
}

static void
start_udp_reader(void)
{
    int  rc;
    int  loops;

    VPLDetachableThreadHandle_t thread;

    udp_reader_started = false;

    rc = VPLDetachableThread_Create(&thread, udp_reader, NULL, NULL, "udp");

    if (rc != 0) {
        log("*** I couldn't create the udp reader thread.\n");
        exit(1);
    }

    VPLDetachableThread_Detach(&thread);
    loops = 0;

    while (!udp_reader_started && loops++ < 10) {
        sleep(1);
    }

    if (!udp_reader_started) {
        log("*** The udp reader thread failed to start.\n");
        exit(1);
    }
}

static void
print_device_state(uint64_t device_id)
{
    int  loops;

    target_state = -1;

    request_device_state(main_client, 0, &device_id, 1, "print_device_state");

    loops = 0;

    while (target_state == -1 && loops++ < 10) {
        sleep(2);
    }

    if (target_state == -1) {
        log("the device query failed.\n");
    } else {
        log("Device %lld is in the \"%s\" state\n",
            (long long) device_id, get_state_text(target_state));
    }
}

/*
 *  Wait for a device to enter a specified state.
 */
static void
wait_for_target_state(int state, const char *where, int send)
{
    int  loops = 0;
    int  result;

    if (sleep_packet == null) {
        log("No sleep packet has been received.\n");
    }

    while (loops++ < 30 && target_state != state) {
        if (send && sleep_packet != null) {
            result = sendto(udp_fd, sleep_packet, sleep_length, 0,
                        (struct sockaddr *) &udp_server_address, udp_server_length);
            print_socket_name(udp_fd);

            if (result < 0) {
                ts();
                fflush(stdout);
                perror("sendto");
            }
        }

        sleep(2);
    }

    if (target_state != state) {
        log("*** Device %lld didn't enter the \"%s\" state - %s\n",
            (long long) target_device_id, get_state_text(state), where);
        print_device_state(target_device_id);
        log("*** Device %lld didn't enter the \"%s\" state - %s\n",
            (long long) target_device_id, get_state_text(state), where);
        exit(1);
    }

    fflush(stdout);
}

/*
 *  Force the target device into the offline state.  To do this, we open a
 *  connection for the device and then close it, making sure that the state
 *  transitions are seen at the server.
 */
static void
reset_target(ans_open_t *input, const char *where)
{
    int  loops;

    /*
     *  Open the target device to remove the sleep entry, if present.  Wait
     *  for the online state.
     */
    target_state_ops = 0;
    target_client = ans_open(input);

    if (!ans_setSubscriptions(target_client, 0, req_device_list, req_device_count)) {
        log("*** set subscriptions failed\n");
        exit(1);
    }

    wait_for_target_state(DEVICE_ONLINE, "target cleanup 1", false);

    ans_requestSleepSetup(target_client, 0, ioac_proto, mac_address,
        sizeof(mac_address));

    loops = 0;

    while (sleep_packet == NULL && loops++ < 20) {
        sleep(1);
    }

    if (sleep_packet == NULL) {
        log("*** No sleep packet was received for the target.\n");
        exit(1);
    }

    loops = 0;

    while (target_state_ops == 0 && loops++ < 20) {
        sleep(1);
    }

    if (target_state_ops == 0) {
         log("*** No state updates were received for the target.\n");
         exit(1);
    }

    /*
     *  Now close the device and wait for the offline state.
     */
    ans_close(target_client, VPL_TRUE);
    wait_for_target_state(DEVICE_OFFLINE, "target cleanup 2", false);
}

static void
start_wakeup_reader(int count)
{
    wakeups_to_receive = count;
    start_udp_reader();
    sleep(2);
}

/*
 *  Wait for a specified number of udp packets to arrive. This routine
 *  synchronizes with the udp_reader method.  If requested, this routine
 *  sends sleep packets periodically, if that's part of the test being
 *  performed.
 */
static void
wait_for_wakeups(int total, int send_sleep, const char *where)
{
    int  loops;
    int  remaining;

    loops      = 0;
    remaining  = wakeups_to_receive;

    log("waiting for %d wakeup packets\n", (int) total);
    print_socket_name(udp_fd);

    while (wakeups_to_receive > 0 && loops < 40) {
        if (send_sleep) {
            sendto(udp_fd, sleep_packet, sleep_length, 0,
                (struct sockaddr *) &udp_server_address,
                udp_server_length);
        }

        sleep(2);

        if (remaining == wakeups_to_receive) {
            loops++;
        } else {
            loops = 0;
            remaining = wakeups_to_receive;
        }
    }

    /*
     *  Check whether enough sleep packets arrived.
     */
    if (wakeups_to_receive > 0) {
        log("*** %d of %d wakeup packets were missed for device %lld - %s.\n",
            (int) remaining, (int) total, (long long) target_device_id, where);
        exit(1);
    }
}

/*
 *  This procedure implements the main testing of the sleep, wakeup, and
 *  device status code.
 */
static void
test_device_status(ans_open_t *input)
{
    struct hostent *  address;

    log("====== Starting the device state test.\n");

    /*
     *  Reset the target device, just to be sure that we can.
     */
    reset_target(input, "status test start");

    /*
     *  Open a UDP socket and create an address pointer to the ANS server
     *  so that we can send sleep packets.
     */
    address = gethostbyname(udp_host);

    if (address == NULL) {
        ts();
        fflush(stdout);
        perror("gethostbyname");
        log("*** No host\n");
        exit(1);
    }

    memset(&udp_server_address, 0, sizeof(udp_server_address));
    memcpy(&udp_server_address.sin_addr, address->h_addr, address->h_length);

    udp_server_address.sin_family = AF_INET;
    udp_server_length             = sizeof(udp_server_address);
    udp_server_address.sin_port   = htons(input->server_tcp_port);

    log("using sleep port %d\n", (int) input->server_tcp_port);

    udp_fd = socket(PF_INET, SOCK_DGRAM, 0);

    if (udp_fd < 0) {
        log("*** socket() failed\n");
        exit(1);
    }

    /*
     *  Send some sleep packets and wait for the device to enter
     *  the sleep state.
     */
    wait_for_target_state(DEVICE_SLEEPING, "sleep send test", true);

    log("====== Testing the wakeup command.\n");

    /*
     *  Okay, have the server send some wakeup packets, then wait until
     *  we get a few.
     */
    start_wakeup_reader(3);
    ans_requestWakeup(main_client, target_device_id);

    log("wait for wakeups for device %lld.\n",
        (long long) target_device_id);

    wait_for_wakeups(3, false, "initial phase");

    /*
     *  Open and close the target device to remove the sleep
     *  entry.  Wait for the offline state.
     */
    log("====== Resetting the target.\n");
    reset_target(input, "mid-testing");

    /*
     *  Now check that retries work.  Send the sleep packet
     *  and then wait for the device to come into the sleep
     *  state.
     */
    log("running the wakeup retry test.\n");

    wait_for_target_state(DEVICE_SLEEPING, "wakeup retry test", true);
    start_wakeup_reader(4);
    ans_requestWakeup(main_client, target_device_id);

    log("wait for wakeups for device %lld - wakeup retry test.\n",
        (long long) target_device_id);
    wait_for_wakeups(4, false, "wakeup retry test");

    reset_target(input, "end of testing");
    log("====== Finished the device status test.\n");
}

static void
test_messaging(ans_open_t *input)
{
    int  pass;
    int  loops;

    log("====== Starting the messaging test.\n");
    target_client  = ans_open(input);

    loops = 0;

    while (!target_online && loops++ < 10) {
        sleep(1);
    }

    if (!target_online) {
        log("*** The target didn't come online!\n");
        exit(1);
    }

    responses = 0;

    pass = ans_sendUnicast(main_client, user_id, target_device_id,
            unicast_text, strlen(unicast_text), 11111101);

    if (!pass) {
        log("*** ans_sendUnicast failed!\n");
        exit(1);
    }

    pass = ans_sendMulticast(main_client, user_id,
            multicast_text, strlen(multicast_text), 222222203);

    if (!pass) {
        log("*** ans_sendMulticast failed!\n");
        exit(1);
    }

    loops = 0;

    while ((unicasts < 1 || multicasts < 2 || responses < 2) && loops++ < 10) {
        sleep(1);
    }

    if (unicasts < 1 && multicasts < 2) {
        log("*** The device-to-device messages were not received!\n");
        exit(1);
    }

    if (responses < 2) {
        log("*** The message responses were not received!\n");
        exit(1);
    }

    ans_close(target_client, true);

    loops = 0;

    while (target_online && loops++ < 5) {
        sleep(1);
    }

    if (target_online) {
        log("*** The target didn't go offline!\n");
        exit(1);
    }

    log("====== Finished the messaging test.\n");
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

#undef max
#define max(a, b) ((a) > (b) ? (a) : (b))

static void
test_cipher(const char *key, int key_length, const char *plaintext_in, const char *ciphertext_in)
{
    ans_client_t  client;

    char *  ciphertext;
    char *  plaintext;
    char *  decrypted;
    int     decrypted_length;
    int     cipher_length;
    int     plain_length;
    int     success;
    int     i;

#ifdef ans_verbose
    log("got plaintext  %s\n", plaintext_in);
    log("got ciphertext %s\n", ciphertext_in);
#endif

    ciphertext    = unmarshal(ciphertext_in, &cipher_length);
    plaintext     = unmarshal(plaintext_in,  &plain_length);
    client.crypto = create_crypto(key, key_length);

    success = ans_decrypt(&client, &decrypted, &decrypted_length, ciphertext, cipher_length);

    if (!success) {
        log("*** The decryption call failed.\n");
        exit(1);
    }

    if (decrypted_length != plain_length) {
        log("*** The decrypted length was incorrect.\n");
        exit(1);
    }

    for (i = 0; i < decrypted_length; i++) {
        if (decrypted[i] != plaintext[i]) {
            log("*** The decrypted text didn't match.\n");
            exit(1);
        }
    }

    free_crypto(&client.crypto);
    free(decrypted);
    free(ciphertext);
    free(plaintext);
    log("The cipher test passed!\n");
}

static void
send_timed_ping(ans_client_t *client)
{
    uint64_t    ping_data[5] = { 0, 1, 2, 3, 4 };
    describe_t  describe;

    clear_describe(&describe);
    describe.type  = Send_timed_ping;
    describe.data  = (void *) ping_data;
    describe.count = sizeof(ping_data);

    send_message(client, &describe);
}

static void
run_main_test(int expected)
{
    int   max_loops = 10;
    int   loops;
    int   last_count;
    int   updates_expected;
    int   send_timed;

    log("====== Running the main test.\n");

    last_count = received_count;
    loops      = max_loops;
    send_timed = true;

    do {
        sleep(2);
        fflush(stdout);

        if (received_count == last_count) {
            loops--;
        }

        if (received_count > 0 && do_ping) {
            send_ping(main_client);
            do_ping = false;
        }

        if (received_count > 0 && send_timed) {
            send_timed_ping(main_client);
            send_timed = false;
        }

        if (received_count > 0 && do_test_misc) {
            test_misc();
            do_test_misc = false;
        }

        if (received_count > 0 && do_sleep_test) {
            log("requesting the sleep setup\n");

            if (ans_requestSleepSetup(main_client, 0, ioac_proto, null, 0)) {
                log("*** ans_requestSleepSet passed with a null mac\n");
                exit(1);
            }

            if (ans_requestSleepSetup(main_client, 0, ioac_proto, invalid_mac,
                sizeof(invalid_mac))) {
                log("*** ans_requestSleepSet passed with a long mac\n");
                exit(1);
            }

            ans_requestSleepSetup(main_client, 0, ioac_proto, mac_address,
                sizeof(mac_address));
            do_sleep_test = false;
        }

        last_count = received_count;

        /*
         *  Check that some test didn't leave the "connection failure okay" flag
         *  set.
         */
        if (main_down_okay) {
            log("The \"main_down_okay\" flag is set during normal testing.\n");
            exit(1);
        }
    } while (received_count < expected && loops > 0);

    if (do_ping) {
        send_ping(main_client);
    }

    if (received_count != expected) {
        log("ans_circle timed out waiting for messages.\n");
        log("*** I got %d messages, but expected %d\n",
            (int) received_count, (int) expected);
        exit(1);
    }

    loops = 10;

    /*
     *  Make sure that we get a reasonable number of updates.  This code is
     *  dependent on the implementation of the ANS server.  It assumes that
     *  the client will receive one update for each valid subscription.
     *  In addition, the test_misc() routine sends one device list query.
     *  Note that a device will not get updates about itself, even if its
     *  own device id is on the subscription list.
     */
    updates_expected = 1 + array_size(req_device_list) - 1;

    while (device_updates < updates_expected && loops-- > 0) {
        sleep(2);
    }

    if (device_updates < updates_expected) {
        log("*** ans_circle timed out waiting for the device state.\n");
        log("got %d, expected %d\n", (int) device_updates,
            (int) updates_expected);
        exit(1);
    }

    loops = 0;

    while (sleep_setups == 0 && loops++ < 5) {
        sleep(1);
    }

    if (sleep_setups == 0) {
        log("*** No sleep setup packets were received\n");
        exit(1);
    }

    if (logins_completed == 0) {
        log("*** No login completion callbacks were received.\n");
        exit(1);
    }

    log("====== Finished the main test.\n");
}

static void
setup_main_callbacks(ans_callbacks_t *callbacks)
{
    callbacks->connectionActive    = t_connection_active;
    callbacks->receiveNotification = t_receive_notification;
    callbacks->receiveSleepInfo    = t_receive_sleep_info_main;
    callbacks->receiveDeviceState  = t_receive_device_state;
    callbacks->connectionDown      = t_connection_down_main;
    callbacks->connectionClosed    = t_connection_closed_main;
    callbacks->setPingPacket       = t_set_ping_packet;
    callbacks->rejectCredentials   = t_reject_credentials;
    callbacks->loginCompleted      = t_login_completed;
    callbacks->rejectSubscriptions = t_reject_subscriptions;
    callbacks->receiveResponse     = t_receive_response;
}

static void
setup_target_callbacks(ans_callbacks_t *callbacks)
{
    callbacks->connectionActive    = t_connection_active_target;
    callbacks->receiveNotification = t_receive_notification;
    callbacks->receiveSleepInfo    = t_receive_sleep_info_target;
    callbacks->receiveDeviceState  = t_receive_device_state_target;
    callbacks->connectionDown      = t_connection_down_target;
    callbacks->connectionClosed    = t_connection_closed_target;
    callbacks->setPingPacket       = t_set_ping_packet;
    callbacks->rejectCredentials   = t_reject_credentials;
    callbacks->loginCompleted      = t_login_completed;
    callbacks->rejectSubscriptions = t_reject_subscriptions;
    callbacks->receiveResponse     = t_receive_response;
}

static char *
hex(const char *input, int length)
{
    char *  output;
    char    buffer[20];

    output = (char *) malloc(length * 2 + 1);

    for (int i = 0; i < length; i++) {
        sprintf(buffer, "%02x", input[i] & 0xff);

        output[i * 2]     = buffer[0];
        output[i * 2 + 1] = buffer[1];
    }

    output[length * 2] = 0;
    return output;
}

static void
test_one_encryption
(
    ans_client_t *client, const char *plaintext, int plain_length
)
{
    char     iv[aes_block_size];
    char *   ciphertext;
    char *   key;
    int      cipher_length;
    int      success;
    char *   key_hex;
    char *   plaintext_hex;
    char *   ciphertext_hex;

    for (int i = 0; i < array_size(iv); i++) {
        iv[i] = i *31 + 3;
    }

    success = ans_encrypt(client, &ciphertext, &cipher_length, iv, plaintext,
                    plain_length);

    if (!success) {
        log("*** ans_encrypt failed in test_one_encryption\n");
        exit(1);
    }

    key = (char *) client->crypto->aes_key;

    key_hex        = hex(key,        aes_block_size);
    plaintext_hex  = hex(plaintext,  strlen(plaintext));
    ciphertext_hex = hex(ciphertext, cipher_length);

    printf("Key: %s\n",        key_hex);
    printf("Plaintext: %s\n",  plaintext_hex);
    printf("Ciphertext: %s\n", ciphertext_hex);

    free(ciphertext);
    free(key_hex);
    free(plaintext_hex);
    free(ciphertext_hex);
}

static const char *test_strings[] =
    {
        "short",
        "0123456789a1234",  /* exactly one aes block */
        "0123456789a12345", /* just over one aes block */
        "longer string that covers multiple blocks",
        "1"
    };

static void
test_encryption(ans_client_t *client)
{
    log("====== Running the encryption test.\n");

    for (int i = 0; i < array_size(test_strings); i++) {
        test_one_encryption(client, test_strings[i], strlen(test_strings[i]));
    }

    log("====== Finished the encryption test.\n");
}

static void
get_key(char **key, int *key_length, const char *source)
{
    *key = unmarshal(source,  key_length);
}

static ans_client_t *
open_central(const char *cluster, const ans_callbacks_t *callbacks_main,
                const char *blob, int blob_length)
{
    ans_client_t *   client;
    ans_open_t       input;
    ans_callbacks_t  callbacks;

    char *  t_blob;
    char *  t_key;

    /*
     *  Set up an input structure for ans_open.
     */
    memset(&input, 0, sizeof(input));
    callbacks = *callbacks_main;

    t_blob = (char *) malloc(blob_length);
    memcpy(t_blob, blob, blob_length);

    t_key = (char *) malloc(key_length);
    memcpy(t_key, key, key_length);

    input.clusterName      = cluster;
    input.callbacks        = &callbacks;
    input.blob             = t_blob;
    input.blobLength       = blob_length;
    input.key              = t_key;
    input.keyLength        = key_length;
    input.deviceType       = "default";
    input.application      = "ans_circle";
    input.verbose          = true;
    input.server_tcp_port  = SERVER_TCP_PORT;

    /*
     *  Open the client.
     */
    client = ans_open(&input);

    free(t_blob);
    free(t_key);

    return client;
}

static void
test_bad_blob(const char *ans_host, const ans_callbacks_t *callbacks)
{
    ans_client_t *  rejected;

    char  junk[80];
    int   i;

    ans_reject_count = 0;
    main_down_okay   = true;
    expecting_reject = true;

    memset(junk, 0, sizeof(junk));
    rejected = open_central(ans_host, callbacks, junk, sizeof(junk));

    i = 0;

    while ((ans_reject_count == 0 || expecting_reject) && i++ < 20) {
        sleep(1);
    }

    if (ans_reject_count == 0) {
        log("*** I didn't receive a rejection.\n");
        exit(1);
    }

    if (expecting_reject) {
        log("*** I didn't receive a rejection callback.\n");
        exit(1);
    }

    ans_close(rejected, VPL_TRUE);

    expecting_reject = false;
    main_down_okay   = false;
}

static void
open_main(const char *cluster, const ans_callbacks_t *callbacks_main)
{
    main_client = open_central(cluster, callbacks_main, blob, blob_length);
}

static int
test_server_timeout(limits_t *limits)
{
    int32_t   my_socket;
    int       tries;
    int64_t   foreground;
    int64_t   wait_time;

    log("====== Testing the server timeout with client %p\n", main_client);

    /*
     *  First, make sure we have a live ans client with a working
     *  connection that is doing keep-alive.
     */
    tries = 0;

    while (!main_client->keep.keeping && tries < 30) {
        sleep(1);
    }

    if (!main_client->keep.keeping) {
        log("The client won't enter keep-alive\n");
        exit(1);
    }

    /*
     *  Make sure at least one packet has been sent, to start the
     *  server clock.
     */
    my_socket = main_client->keep.live_socket_id;
    ans_keep_packets_out = 0;
    tries = 0;

    while (ans_keep_packets_out == 0 && tries++ < 30) {
        sleep(1);
    }

    if (ans_keep_packets_out == 0) {
        log("The client won't send keep-alive packets\n");
        exit(1);
    }

    /*
     *  Make sure that a keep-alive packet is received for this
     *  connection.  The ANS server won't start keep-alive until
     *  a packet has been received.  The server sends a keep-alive
     *  packet when it receives one for the connection.
     */
    log("ANS connection " FMTs64 ", socket id %d, device " FMTu64 "\n",
              main_client->connection,
        (int) my_socket,
              main_client->device_id);

    ans_keep_packets_in = 0;

    tries = 0;

    while (ans_keep_packets_in == 0 && tries < 30) {
        sleep(1);
    }

    if (ans_keep_packets_in == 0) {
        log("The client hasn't received any packets from the server\n");
        exit(1);
    }

    /*
     *  Set the ping time to zero to disable the software keep-alive,
     *  and ping the keep-alive thread so that it eventually notices the
     *  parameter change.
     */
    ans_base_interval = 0;
    ping_keep_alive(main_client);

    /*
     *  Now wait for keep-alive to stop.
     */
    tries = 0;

    while (main_client->keep.keeping && tries++ < 30) {
        sleep(1);
        ping_keep_alive(main_client);
    }

    if (main_client->keep.keeping) {
        log("The client won't stop keep-alive!\n");
        exit(1);
    }

    if (my_socket != main_client->keep.live_socket_id) {
        log("The server connection failed during timeout testing!\n");
        exit(1);
    }

    log("The client stopped keep-alive.\n");

    /*
     *  Wait for the connection to die.
     */
    main_down_okay = true;

    /*
     *  These formulae are from the ANS server.
     */
    foreground = limits->base_delay + limits->retry_delay * limits->retry_count;

    if (main_client->enable_tcp_only) {
        wait_time = (max(limits->factor, 1) + 1) * foreground;
    } else {
        wait_time = foreground;
    }

    wait_time += VPLTime_FromSec(1);        // make a conservate RTT estimate
    wait_time += VPLTime_FromSec(1) / 2;    // round up
    wait_time  = wait_time /  VPLTime_FromSec(1) + 2;

    log("wait_time is %d seconds\n", (int) wait_time);

    while (my_socket == main_client->keep.live_socket_id && wait_time-- > 0) {
        if (main_client->keep.keeping) {
            log("Software keep-alive is active again!\n");
            return false;
        }

        sleep(1);
    }

    /*
     *  Try to encourage TCP to fail if the connection is down.
     */
    send_ping(main_client);
    sleep(1);

    if (my_socket == main_client->keep.live_socket_id) {
        log("The server timeout code failed!\n");
        sleep(10);
        log("The server timeout code failed!\n");
        log("Done.\n");
        exit(1);
    }

    main_down_okay = false;
    return true;
}

int
main(int argc, char **argv, char **envp)
{
    const char *  ans_host;
    const char *  device_host;
    const char *  ans_user        = getenv("ans_user");
    const char *  ans_device      = getenv("ans_device");
    const char *  ans_blob        = getenv("ans_blob");
    const char *  ans_target_blob = getenv("ans_target_blob");
    const char *  ans_key         = getenv("ans_key");
    const char *  ans_target_key  = getenv("ans_target_key");
    const char *  expect          = getenv("dt_expect");

    const char *  ciphertext      = getenv("ans_ciphertext");
    const char *  plaintext       = getenv("ans_plaintext");

    ans_callbacks_t  callbacks_main;
    ans_callbacks_t  callbacks_target;
    ans_open_t       input_target;

    uint64_t  query_device;
    int       expected = 8;
    char *    end;
    int       die;
    int       tries;
    int       pass;
    limits_t  limits;

    die = false;

    query_device       = (uint64_t) 8 * 1000 * 1000 * 1000;
    req_device_list[0] = query_device;
    req_device_list[1] = query_device + (uint64_t) 1000 * 1000 * 1000;

    ans_host    = getenv("ans_host");
    device_host = getenv("device_host");
    target_host = getenv("target_host");
    udp_host    = getenv("udp_host");

    setlinebuf(stdout);

    if (ans_host == NULL) {
        log("ans_host wasn't set in the environment.\n");
        die = VPL_TRUE;
    }

    if (ans_user == NULL || ans_device == NULL) {
        log("ans_user and ans_device must be set in the environment.\n");
        die = VPL_TRUE;
    }

    if (device_host == NULL) {
        device_host = ans_host;
    }

    if (target_host == null) {
        target_host = device_host;
    }

    if (udp_host != NULL) {
        ;
    } else if (device_host != NULL) {
        udp_host = device_host;
    } else {
        udp_host = ans_host;
    }

    if (ans_key == null || ans_target_key == NULL) {
        log("ans_key and ans_target_key must be set in the environment.\n");
        die = VPL_TRUE;
    }

    if (ans_blob == null || ans_target_blob == NULL) {
        log("ans_blob and ans_target_blob must be set in the environment.\n");
        die = VPL_TRUE;
    }

    if (die) {
        exit(1);
    }

    user_id            = strtoll(ans_user, NULL, 10);
    main_device_id     = strtoll(ans_device, NULL, 10);
    target_device_id   = main_device_id + 1;
    req_device_list[0] = main_device_id;
    req_device_list[1] = target_device_id;

    log("got blob: %s\n", ans_blob);
    log("got key:  %s\n", ans_key);

    blob = unmarshal(ans_blob, &blob_length);

    if (blob == NULL) {
        log("*** The blob malloc failed.\n");
        exit(1);
    }

    get_key(&key, &key_length, ans_key);

    if (plaintext != NULL && ciphertext != NULL) {
        test_cipher(key, key_length, plaintext, ciphertext);
    }

    free(key);
    log("user id " FMTu64 ", main device id " FMTu64 "\n",
        user_id, main_device_id);

    if (expect != NULL) {
        expected = strtol(expect, &end, 10);

        if (*end != 0) {
            log("*** $dt_expect is not a valid integer.\n");
            exit(1);
        }
    }

    log("Hosts:\n");
    log("    component  %s\n", ans_host);
    log("    device     %s\n", device_host);
    log("    target     %s\n", target_host);
    log("    udp        %s\n", udp_host);

    do_test_misc  = true;
    do_ping       = getenv("ans_no_ping")   == NULL;
    do_sleep_test = getenv("ans_no_sleep")  == NULL;

    if (!do_sleep_test) {
        sleep_setups++;
    }

    log("Expecting %d messages.\n", expected);

    setup_main_callbacks(&callbacks_main);
    setup_target_callbacks(&callbacks_target);
    get_key(&key, &key_length, ans_key);

    test_bad_blob(ans_host, &callbacks_main);
    open_main(device_host, &callbacks_main);

    free(blob);
    free(key);

    /*
     *  Add another device so that we can watch its status.  First, we
     *  need to unmarshal the blob and key for the target.
     */
    blob = unmarshal(ans_target_blob, &blob_length);
    get_key(&key, &key_length, ans_target_key);

    /*
     *  Get ready to open the target device for the messaging test.
     */
    memset(&input_target, 0, sizeof(input_target));

    input_target.clusterName      = target_host;
    input_target.callbacks        = &callbacks_target;
    input_target.blob             = blob;
    input_target.blobLength       = blob_length;
    input_target.key              = key;
    input_target.keyLength        = key_length;
    input_target.deviceType       = "default";
    input_target.application      = "ans_circle";
    input_target.verbose          = true;
    input_target.server_tcp_port  = SERVER_TCP_PORT;

    test_messaging(&input_target);
    run_main_test(expected);

    /*
     *  Test the sleep code and the subscripton updates.  Update the
     *  ans_open_t to point to the target callbacks and invoke the
     *  test.
     */
    test_device_status(&input_target);

    free(blob);
    free(key);

    test_encryption(main_client);

    /*
     *  Check that some test didn't leave the "connection failure okay" flag
     *  set.
     */
    if (main_down_okay) {
        log("The \"main_down_okay\" flag is set during normal testing.\n");
        exit(1);
    }

    /*
     *  Okay, try to force a software-level timeout if software
     *  keep-alive is enabled.
     */
    log("====== Starting the server software timeout test\n");
    main_client->mt.inited = false;  // shut off jitter
    get_limits(&main_client->keep, &limits);

    if (limits.valid) {
        tries = 0;

        do {
            log("====== Starting a timeout test\n");
            pass = test_server_timeout(&limits);
            log("=== test_server_timeout returned %s\n", pass ? "success" : "failure");
        } while (!pass && tries++ < 4);

        if (!pass) {
            log("*** The server timeout test failed.\n");
            exit(1);
        }
    }

    log("====== Finished all the tests!\n");

    main_down_okay = true;
    exiting        = true;

    ans_close(main_client, VPL_TRUE);

    if (ans_lock_errors != 0) {
        log("%d lock errors occurred!\n", (int) ans_lock_errors);
        exit(1);
    }

    free((void *) sleep_packet);
    printf("Test passed.\n");
    return 0;
}

#undef VPLAssert_Failed

void
VPLAssert_Failed(char const* file, char const* func, int line, char const* formatMsg, ...)
{
}
