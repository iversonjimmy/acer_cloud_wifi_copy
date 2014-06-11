//
//  Copyright 2011-2013 Acer Cloud Technology.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF Acer Cloud Technology.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF Acer Cloud Technology.
//

#include <vpl_types.h>
#include <pxd_mtwist.h>

#undef  size
#define size(mt)  (sizeof(mt->state) / sizeof(mt->state[0]))

/*
 *  Implement the basic Mersenne Twister pseudo-random number
 *  generator.
 */
void
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
int32_t
mtwist_next(mtwist_t *mt, int32_t limit)
{
    uint32_t  next;

    if (limit == 1 || limit == 0) {
        return 0;
    }

    next = mtwist_rand(mt);

    if (limit > 1) {
        next &= 0x7fffffff; /* make next non-negative */
        next %= limit;
    }

    return next;
}

uint32_t
mtwist_rand(mtwist_t *mt)
{
    uint32_t  next;

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

    return next;
}
