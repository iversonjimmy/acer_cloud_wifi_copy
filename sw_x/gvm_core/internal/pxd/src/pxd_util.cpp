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
#include <pxd_log.h>
#include <pxd_util.h>

static int  pxd_lock_errors;

void
pxd_mutex_init(VPLMutex_t *mutex)
{
    int  result;

    result = VPLMutex_Init(mutex);

    if (result != VPL_OK) {
        log_error("VPLMutex_Init failed: %d", result);
        pxd_lock_errors++;
    }
}

void
pxd_mutex_lock(VPLMutex_t *mutex)
{
    int  result;

    result = VPLMutex_Lock(mutex);

    /*
     *  We have nothing useful to do if we can't get the lock.  We should
     *  abort the program, most likely.
     */
    if (result != VPL_OK) {
        log_error("VPLMutex_Lock failed: %d", result);
        pxd_lock_errors++;
    }
}

void
pxd_mutex_unlock(VPLMutex_t *mutex)
{
    VPLMutex_Unlock(mutex);
}

static const char * digits = "0123456789";

/*
 *  Convert a network address into a hex string.
 */
void
pxd_convert_address(VPLNet_addr_t address, char *buffer, int max_length)
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
