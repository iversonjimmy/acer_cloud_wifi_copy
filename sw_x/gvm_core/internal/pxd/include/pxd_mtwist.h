//
//  Copyright 2011-2013 Acer Cloud Technology
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF Acer Cloud Technology.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF Acer Cloud Technology.
//

#ifndef PXD_MTWIST_H
#define PXD_MTWIST_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  We use a Mersenne twister to generate pseudo-random numbers as needed.
 *  This structure defines an instance of a Mersenne twister.
 */
typedef struct {
    int       index;
    int       inited;
    uint32_t  state[623];
} mtwist_t;

void      mtwist_init(mtwist_t *, uint32_t);
int32_t   mtwist_next(mtwist_t *, int32_t );
uint32_t  mtwist_rand(mtwist_t *          );

#ifdef __cplusplus
}
#endif

#endif /* PXD_MTWIST_H */
