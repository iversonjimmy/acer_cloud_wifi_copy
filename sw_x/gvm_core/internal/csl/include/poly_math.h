/*
 *               Copyright (C) 2005, BroadOn Communications Corp.
 *
 *  These coded instructions, statements, and computer programs contain
 *  unpublished  proprietary information of BroadOn Communications Corp.,
 *  and  are protected by Federal copyright law. They may not be disclosed
 *  to  third  parties or copied or duplicated in any form, in whole or in
 *  part, without the prior written consent of BroadOn Communications Corp.
 *
 */
/* 
 * poly_math.h
 * definitions for binary arithmetic: characteristic two,
 * vector length n: field2n.
 * number of bits for field fixed here
 */


#ifndef __POLY_MATH_H__
#define __POLY_MATH_H__


#define NUM_BITS 233 
#define WORD_SIZE (sizeof(int) *8) 

/* to determine size of array of big int structure
 */
#define NUM_WORD (NUM_BITS/WORD_SIZE) 

/* number of shifts needed to get to MSB of zero 
 * offset of polynomial coeff list 
 */
#define UPR_SHIFT (NUM_BITS % WORD_SIZE) 

/* number of machine words to 
 * hold polynomial or large int 
 */
#define MAX_LONG (NUM_WORD + 1) 

#define MAX_BITS (MAX_LONG *WORD_SIZE) 

#define MAX_SHIFT (WORD_SIZE -1)

/* mask for most significant bit */
#define MSB ((element)(1UL << MAX_SHIFT))

#define UPR_BIT  (1L << (UPR_SHIFT -1))
#define UPR_MASK (~ (-1L << UPR_SHIFT))

/* byte stream representing field element */
#define OCTET_STRING_MASK (0xffff) /* rounding up to nearest byte to cover
                                      bits in the MSB */
#define OCTET_STRING_SHIFT (16)    /* shift corresponding to 0xffff */
#define OCTET_STRING_LEN (30)      /* NUM_BITS/WORD_SIZE + sizeof(0xffff) */
#define BYTE_SHIFT       (8)      
typedef unsigned int element;     /* single word */

/* the binary field representation */
typedef struct {
    element e[MAX_LONG];
} field_2n;


typedef enum { CSL_TRUE = 0, 
               CSL_FALSE
}field_boolean;


#define DOUBLE_BITS ( 2 * NUM_BITS)
#define DOUBLE_WORD (DOUBLE_BITS / WORD_SIZE)
#define MAX_DOUBLE (DOUBLE_WORD + 1)

/* structure for convenience in FG 2^m arithmetic operations */
typedef struct {
	element e[MAX_DOUBLE];
} field_double;

/* function prototypes */

void poly_copy(const field_2n *from, field_2n *to);
void poly_null(field_2n *a);
void poly_sqr(field_2n *a, field_double *b);
void poly_mul_partial(field_2n *a, field_2n *b, field_double *c);
void poly_div_shift(field_double *a);
void poly_log2(element x, short int *result);
void poly_degree_of(element *t, short int dim, short int *result);
void poly_div(field_double *top, field_2n *bottom, field_2n *quotient, field_2n *remainder);
void poly_mul_fast(field_2n *a, field_2n *b, field_2n *c);
void poly_mul(field_2n *a, field_2n *b, field_2n *c);
void poly_inv(field_2n *a, field_2n *inverse);
void poly_rot_right(field_2n *a);
#endif /* __POLY_MATH_H__ */
