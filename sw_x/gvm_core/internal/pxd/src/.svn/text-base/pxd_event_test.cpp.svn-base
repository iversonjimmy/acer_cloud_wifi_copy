//
//  Copyright 2011-2013 Acer Cloud Technology.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF Acer Cloud Technology.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF Acer Cloud Technology.
//

#include <stdio.h>
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
#include <stdlib.h>

#include "pxd_test.h"

static int          my_VPLSocket_Recv(VPLSocket_t socket, void* buf, int len);
static VPLSocket_t  my_VPLSocket_Create(int a1, int a2, int a3);


static int   fail_receive  = false;
static int   fail_create   = false;

#define VPLSocket_Recv    my_VPLSocket_Recv
#define VPLSocket_Create  my_VPLSocket_Create

#include "pxd_event.cpp"
#include "pxd_util.cpp"
#include "pxd_log.cpp"

#undef VPLSocket_Recv
#undef VPLSocket_Create

static void
check_peer(pxd_event_t *event)
{
    if (VPLSocket_GetPeerPort(event->socket) != VPLSocket_GetPort(event->out_socket)) {
        log("test: *** GetPeerPort and GetPort disagree!\n");
        exit(1);
    }

    if (VPLSocket_GetPeerAddr(event->socket) != VPLSocket_GetAddr(event->out_socket)) {
        log("test: *** GetPeerAddr and GetAddr disagree!\n");
        exit(1);
    }
}

int
main(int argc, char **argv)
{
    pxd_event_t       event;
    VPLSocket_poll_t  poll;
    int               i;
    int               delay;
    pxd_event_t *     output;

    printf("Starting the pxd event test.\n");

    /*
     *  Make an event structure.  That should work...
     */
    memset(&event, 0, sizeof(event));
    init_event(&event);
    pxd_mutex_init(&event.mutex);
    pxd_event_deaths = 0;

    pxd_mutex_init(&event.mutex);
    make_event(&event);

    if (event.dead || pxd_event_deaths != 0) {
        log("test: *** make_event failed!\n");
        exit(1);
    }

    check_peer(&event);

    /*
     *  Report an I/O and see whether the event is rebuilt.
     */
    memset(&poll, 0, sizeof(poll));
    poll.revents = VPLSOCKET_POLL_ERR;

    pxd_check_event(&event, 0, &poll);

    if (event.dead || pxd_event_deaths != 1) {
        log("test: *** pxd_check_event(error) failed!\n");
        exit(1);
    }

    /*
     *  Simulate an I/O error in pxd_ping and see whether
     *  it's handled properly.
     */
    pxd_force_event  = 1;
    pxd_event_deaths = 0;

    pxd_ping(&event, "test event");

    if (!event.dead || pxd_event_deaths != 1) {
        log("test: *** pxd_ping(error) failed!\n");
        exit(1);
    }

    /*
     *  Now see whether the pxd_check_event code rebuilds the
     *  event structure properly.
     */
    poll.revents = 0;
    pxd_check_event(&event, 0, &poll);
    event.backoff = 500;

    if (event.dead || pxd_event_deaths != 2) {
        log("test: *** pxd_check_event on a dead event failed!\n");
        exit(1);
    }

    free_event(&event);

    for (i = 1; i < 7; i++) {
        pxd_force_make  = i;
        event.backoff   = 0;

        pxd_mutex_init(&event.mutex);
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

    pxd_mutex_init(&event.mutex);
    make_event(&event);
    pxd_event_deaths = 0;
    poll.revents = VPLSOCKET_POLL_RDNORM;

    pxd_check_event(&event, 0, &poll);

    if (pxd_event_deaths != 1) {
        log("test: *** pxd_check_event ignored a receive error!\n");
        exit(1);
    }

    VPLMutex_Destroy(&event.mutex);

    pxd_mutex_init(&event.mutex);
    make_event(&event);
    pxd_events = 0;
    pxd_ping(&event, "post test");
    pxd_event_wait(&event, 1000);

    if (pxd_events != 1) {
        log("test: *** pxd_event_wait didn't see the event.\n");
        exit(1);
    }

    free_event(&event);

    fail_create = true;
    event_max_delay = 1;

    pxd_create_event(&output);

    if (output != null) {
        log("test: *** pxd_create_event didn't fail.\n");
        exit(1);
    }

    fail_create = false;

    if (pxd_lock_errors != 0) {
        log("The test had lock errors.\n");
        exit(1);
    }

    printf("The pxd event test passed.\n");
    return 0;
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

static VPLSocket_t
my_VPLSocket_Create(int a1, int a2, int a3)
{
    if (fail_create) {
        return VPLSOCKET_INVALID;
    } else {
        return VPLSocket_Create(a1, a2, a3);
    }
}
