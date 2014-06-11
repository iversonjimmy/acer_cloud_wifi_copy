//
//  Copyright 2011-2013 Acer Cloud Technology.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF Acer Cloud Technology.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF Acer Cloud Technology.
//

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

#ifndef IOS
#include <malloc.h>
#endif

#include <string.h>
#include <stdio.h>

#undef  true
#undef  false
#undef  null

#define  true    1
#define  false   0
#define  null    0

#define abs(x)         ((x) >= 0 ? (x) : -(x))
#define pxd_min(a, b)      ((a) < (b) ? (a) : (b))
#define pxd_max(a, b)      ((a) > (b) ? (a) : (b))
#define array_size(x)  (sizeof(x) / sizeof((x)[0]))

#define invalid(x)     (VPLSocket_Equal((x), VPLSOCKET_INVALID))
#define valid(x)       (!invalid(x))

#include <pxd_log.h>
#include <pxd_util.h>
#include <pxd_event.h>

/*
 *  Define the structure for controlling a channel for event notification.
 *  Each channel has a socket for sending an event notification, a socket
 *  for receiving notifications, and a server socket for making the TCP/IP
 *  connection that is used.
 *
 *  An "event" is signaled by writing one byte to the outgoing socket.
 */
typedef struct pxd_event_s {
    VPLSocket_t   socket;           /* the socket for listening         */
    VPLSocket_t   server_socket;    /* the server socket for connecting */
    VPLSocket_t   out_socket;       /* the socket for sending an event  */
    volatile int  dead;             /* a flag for a dead event channel  */
    volatile int  backoff;          /* the current retry time           */
    VPLMutex_t    mutex;
    volatile int  pending_event;    /* for debugging */
} pxd_event_t;

static int  pxd_force_make;
static int  pxd_force_event;
static int  pxd_event_deaths;
static int  pxd_events;

static int  event_min_delay =  1;
static int  event_max_delay = 10;

/*
 *  Close a socket, if it's valid, and record that we closed it.
 */
static void
close_event_socket(VPLSocket_t *socket)
{
    if (valid(*socket)) {
        VPLSocket_Close(*socket);
        *socket = VPLSOCKET_INVALID;
    }
}

/*
 *  Send an event to a thread.
 */
void
pxd_ping(pxd_event_t *event, const char *where)
{
    char  buffer;
    int   count;

    buffer  = 0;
    count   = 1;

    pxd_mutex_lock(&event->mutex);
    event->pending_event++;    /* for testing */

    if (valid(event->out_socket)) {
        count = VPLSocket_Send(event->out_socket, &buffer, 1);
    }

    if (count <= 0 || --pxd_force_event == 0) {
        close_event_socket(&event->out_socket);
        pxd_event_deaths++;
        event->dead = true;
        log_error("%s event error:  %d", where, count);
    }

    pxd_mutex_unlock(&event->mutex);
}

/*
 *  Initialize an event structure.  Only a few fields need to be
 *  set.
 */
static void
init_event(pxd_event_t *event)
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
free_event(pxd_event_t *event)
{
    pxd_mutex_lock(&event->mutex);

    close_event_socket(&event->server_socket);
    close_event_socket(&event->socket);
    close_event_socket(&event->out_socket);

    event->dead = true;
    pxd_mutex_unlock(&event->mutex);
}

/*
 *  Compute an exponential back-off time for event creation
 *  retry.  An upper limit is imposed on the wait time.
 */
static int
backoff(pxd_event_t *event)
{
    int  result;

    result         = pxd_max(event->backoff, event_min_delay);
    result         = pxd_min(result,         event_max_delay);
    event->backoff = pxd_min(2 * result,     event_max_delay);

    return result;
}

/*
 *  Try to make a loop-back connection for sending events.
 */
static void
make_event(pxd_event_t *event)
{
    VPLSocket_addr_t  address;

    int  result;

    init_event(event);

    pxd_mutex_lock(&event->mutex);
    event->dead = true;

    /*
     *  First make a server socket to which we can connect.
     */
    event->server_socket = VPLSocket_Create(VPL_PF_INET, VPLSOCKET_STREAM, VPL_FALSE);

    if (invalid(event->server_socket) || --pxd_force_make == 0) {
        pxd_mutex_unlock(&event->mutex);
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

    if (result != VPL_OK || --pxd_force_make == 0) {
        pxd_mutex_unlock(&event->mutex);
        log_error("event bind failure %d", result);
        VPLThread_Sleep(VPLTime_FromSec(backoff(event)));
        free_event(event);
        return;
    }

    /*
     *  Make it possible to receive connections on the socket.
     */
    result = VPLSocket_Listen(event->server_socket, 10);

    if (result != VPL_OK || --pxd_force_make == 0) {
        pxd_mutex_unlock(&event->mutex);
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

    if (invalid(event->out_socket) || --pxd_force_make == 0) {
        pxd_mutex_unlock(&event->mutex);
        VPLThread_Sleep(VPLTime_FromSec(backoff(event)));
        log_error_1("event out_socket creation failure");
        free_event(event);
        return;
    }

    result = VPLSocket_Connect(event->out_socket, &address, sizeof(address));

    if (result != VPL_OK || --pxd_force_make == 0) {
        pxd_mutex_unlock(&event->mutex);
        VPLThread_Sleep(VPLTime_FromSec(backoff(event)));
        log_error("event connection failure %d", result);
        free_event(event);
        return;
    }

    /*
     *  Accept the incoming connection.
     */
    result = VPLSocket_Accept(event->server_socket, &address, sizeof(address), &event->socket);

    if (result != VPL_OK || --pxd_force_make == 0) {
        pxd_mutex_unlock(&event->mutex);
        VPLThread_Sleep(VPLTime_FromSec(backoff(event)));
        log_error("event accept failure %d", result);
        free_event(event);
        return;
    }

    event->dead    = false;
    event->backoff = event_min_delay;

    pxd_mutex_unlock(&event->mutex);
}

/*
 *  Poll to see whether an event has occurred.  Also, check
 *  for socket failures.
 */
void
pxd_check_event(pxd_event_t *event, int result, VPLSocket_poll_t *poll)
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
        } else if (count > 0) {
            pxd_events++;
        }
    }

    /*
     *  If our event socket has died, make a new one if possible.
     */
    if (event->dead) {
        pxd_event_deaths++;
        pxd_mutex_lock(&event->mutex);
        free_event(event);
        make_event(event);
        pxd_mutex_unlock(&event->mutex);
    }
}

/*
 *  Wait for an event to be posted.  A zero or negative time
 *  limit is translated to a 2 second wait.
 */
void
pxd_event_wait(pxd_event_t *event, int64_t wait_limit)
{
    uint64_t  timeout;
    int       result;

    VPLSocket_poll_t  poll[1];

    timeout         = wait_limit > 0 ? wait_limit : VPLTime_FromSec(2000000);
    poll[0].socket  = event->socket;
    poll[0].events  = VPLSOCKET_POLL_RDNORM;
    poll[0].revents = 0;

    result = VPLSocket_Poll(poll, array_size(poll), timeout);

    pxd_check_event(event, result, &poll[0]);
}

int
pxd_create_event(pxd_event_t **eventp)
{
    pxd_event_t *  event;
    int            tries;

    *eventp = null;
    event   = (pxd_event_t *) malloc(sizeof(*event));

    if (event == null) {
        log_error_1("malloc failed in pxd_create_event");
        return false;
    }

    memset(event, 0, sizeof(*event));
    tries = 10;

    do {
        pxd_mutex_init(&event->mutex);
        make_event(event);

        if (event->dead) {
            VPLThread_Sleep(VPLTime_FromMillisec(200));
        }
    } while (event->dead && tries-- > 0);

    if (event->dead) {
        free(event);
        log_error_1("make_event failed in pxd_create_event");
        return false;
    }

    *eventp = event;
    return true;
}

void
pxd_free_event(pxd_event_t **eventp)
{
    pxd_event_t *  event;

    event = *eventp;

    if (event == null) {
        return;
    }

    free_event(event);
    VPLMutex_Destroy(&event->mutex);
    free(event);
    *eventp = null;
}

VPLSocket_t
pxd_socket(pxd_event_t *event)
{
    return event->socket;
}
